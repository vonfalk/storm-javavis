#include "stdafx.h"
#include "Ctor.h"
#include "Type.h"
#include "Cast.h"
#include "Named.h"
#include "Core/Fn.h"
#include "Compiler/Engine.h"
#include "Compiler/Exception.h"

namespace storm {
	namespace bs {

		BSRawCtor::BSRawCtor(Array<ValParam> *params, SrcPos pos)
			: Function(Value(), new (params) Str(Type::CTOR), values(params)),
			  params(params),
			  pos(pos) {

			// If we inherit from a class that does not know which thread to run on, we need a Thread as the
			// first parameter (#1, it is after the this ptr).
			RunOn runOn = params->at(0).type.type->runOn();
			needsThread = runOn.state == RunOn::runtime;
			if (needsThread) {
				Value expected = thisPtr(Thread::stormType(engine()));

				if (params->count() < 2)
					throw SyntaxError(pos, L"This constructor needs to take a " + ::toS(expected)
									+ L" as the first parameter since we are threaded.");

				if (!expected.canStore(params->at(1).type))
					throw SyntaxError(pos, L"This constructor needs to take a " + ::toS(expected) + L" as the first"
									L" parameter, not " + ::toS(params->at(1)));
			}

			// Rename until it is initialized!
			params->at(0).name = new (this) Str(L" this");
			reset();
		}

		void BSRawCtor::reset() {
			// Could be done better...
			setCode(new (this) LazyCode(fnPtr(engine(), &BSRawCtor::generateCode, this)));
		}

		code::Var BSRawCtor::findThread(CodeGen *s, Array<code::Operand> *params) {
			using namespace code;

			RunOn on = runOn();
			if (on.state != RunOn::runtime)
				return Function::findThread(s, params);

			// We know it is always the first parameter!
			code::Var r = s->l->createVar(s->block, Size::sPtr);
			*s->l << mov(r, params->at(1));
			return r;
		}

		CodeGen *BSRawCtor::generateCode() {
			CtorBody *body = createBody();

			using namespace code;
			CodeGen *state = new (this) CodeGen(runOn(), true, Value());
			Listing *l = state->l;

			*l << prolog();

			// Parameters (first one is special).
			{
				code::Var thisVar = state->createParam(params->at(0).type);
				SimplePart *hiddenName = new (this) SimplePart(new (this) Str(L" this"));
				SimplePart *normalName = new (this) SimplePart(new (this) Str(L"this"));
				LocalVar *hidden = body->variable(hiddenName);
				LocalVar *normal = body->variable(normalName);
				hidden->var = VarInfo(thisVar);
				normal->var = VarInfo(thisVar);
			}

			for (nat i = 1; i < params->count(); i++) {
				// Value t = params->at(i).type;
				SimplePart *name = new (this) SimplePart(params->at(i).name);
				LocalVar *var = body->variable(name);
				assert(var);
				var->createParam(state);
			}

			CodeResult *r = CREATE(CodeResult, this);
			body->code(state, r);

			*l << fnRet();

			// PLN(identifier() << L": " << l);
			// PLN(engine().arena()->transform(l, null));
			return state;
		}

		LocalVar *BSRawCtor::addParams(Block *to) {
			LocalVar *thread = 0;

			for (nat i = 0; i < params->count(); i++) {
				LocalVar *var = new (this) LocalVar(params->at(i).name, params->at(i).type, pos, true);
				if (i == 0)
					var->constant = true;
				to->add(var);
				if (i == 1 && needsThread)
					thread = var;
			}

			return thread;
		}

		CtorBody *BSRawCtor::createBody() {
			throw InternalError(L"A BSRawCtor can not be used without overriding 'createBody'!");
		}


		/**
		 * Regular ctor.
		 */

		BSCtor::BSCtor(Array<ValParam> *params, Scope scope, syntax::Node *body, SrcPos pos)
			: BSRawCtor(params, pos), scope(scope), body(body) {}

		CtorBody *BSCtor::createBody() {
			if (!body)
				return defaultParse();

			return syntax::transformNode<CtorBody, BSCtor *>(body, this);
		}

		CtorBody *BSCtor::defaultParse() {
			CtorBody *r = new (this) CtorBody(this, scope);
			Actuals *actual = new (this) Actuals();
			SuperCall *super = new (this) SuperCall(pos, r, actual);
			super->pos = pos;
			r->add(super);
			return r;
		}


		/**
		 * Tree ctor.
		 */

		BSTreeCtor::BSTreeCtor(Array<ValParam> *params, SrcPos pos)
			: BSRawCtor(params, pos) {}

		void BSTreeCtor::body(CtorBody *body) {
			if (root)
				reset();

			root = body;
		}

		CtorBody *BSTreeCtor::createBody() {
			if (!root)
				throw RuntimeError(L"The body of " + ::toS(identifier()) + L"was not set before trying to use it.");

			return root;
		}


		/**
		 * Constructor body.
		 */

		CtorBody::CtorBody(BSCtor *ctor) : ExprBlock(ctor->pos, ctor->scope) {
			threadParam = ctor->addParams(this);
		}

		CtorBody::CtorBody(BSRawCtor *ctor, Scope scope) : ExprBlock(ctor->pos, scope) {
			threadParam = ctor->addParams(this);
		}

		void CtorBody::add(Array<Expr *> *exprs) {
			for (nat i = 0; i < exprs->count(); i++) {
				add(exprs->at(i));
			}
		}

		void CtorBody::blockCode(CodeGen *state, CodeResult *to, const code::Block &block) {
			if (threadParam) {
				thread = state->l->createVar(state->block, Size::sPtr);
				*state->l << mov(thread, threadParam->var.v);
			}

			Block::blockCode(state, to, block);
		}

		Initializer::Initializer(syntax::SStr *name, Expr *expr) : name(name), expr(expr) {}

		Initializer::Initializer(syntax::SStr *name, Actuals *params) : name(name), params(params) {}

		void Initializer::toS(StrBuf *to) const {
			*to << name->v;
			if (params) {
				*to << params;
			} else if (expr) {
				*to << S(" = ") << expr;
			}
		}

		SuperCall::SuperCall(SrcPos pos, CtorBody *block, Actuals *params, Array<Initializer *> *init)
			: Expr(pos) {

			this->init(block, params);
			for (nat i = 0; i < init->count(); i++)
				this->init(init->at(i));
		}

		SuperCall::SuperCall(SrcPos pos, CtorBody *block, Actuals *params) : Expr(pos) {
			init(block, params);
		}

		void SuperCall::init(CtorBody *block, Actuals *params) {
			rootBlock = block;

			initMap = new (this) InitMap();

			// Add the regular this parameter!
			SimplePart *name = new (this) SimplePart(new (this) Str(L" this"));
			thisVar = block->variable(name);
			thisPtr = thisVar->result;
			LocalVar *created = new (this) LocalVar(new (this) Str(L"this"), thisPtr, thisVar->pos, true);
			created->constant = true;
			block->add(created);

			this->params = params;
			LocalVarAccess *l = new (this) LocalVarAccess(pos, thisVar);
			params->addFirst(l);

			scope = block->scope;
		}

		ExprResult SuperCall::result() {
			return ExprResult();
		}

		void SuperCall::init(Initializer *init) {
			Str *name = init->name->v;
			MemberVar *v = as<MemberVar>(thisPtr.type->find(name, new (this) Array<Value>(1, thisPtr), scope));
			if (v == null || v->params->at(0).type != thisPtr.type)
				throw SyntaxError(init->name->pos, L"The member variable " + ::toS(name) + L" was not found in "
								+ ::toS(thisPtr));

			if (initMap->has(name))
				throw SyntaxError(init->name->pos, L"The member " + ::toS(name) + L" has already been initialized.");

			initMap->put(name, init);
		}

		void SuperCall::toS(StrBuf *to) const {
			*to << S("init") << params << S(" {\n");
			to->indent();

			for (InitMap::Iter i = initMap->begin(); i != initMap->end(); ++i) {
				*to << i.v() << S(";\n");
			}

			to->dedent();
			*to << S("}");
		}

		void SuperCall::callParent(CodeGen *s) {
			Type *parent = thisPtr.type->super();
			if (!parent)
				return;

			RunOn runOn = thisPtr.type->runOn();
			bool hiddenThread = runOn.state == RunOn::runtime;
			if (!hiddenThread && parent == TObject::stormType(engine())) {
				callTObject(s, runOn.thread);
				return;
			}

			// Find something to call.
			BSNamePart *values = new (this) BSNamePart(Type::CTOR, pos, params);
			values->alter(0, Value(parent));

			if (hiddenThread)
				values->insert(storm::thisPtr(Thread::stormType(engine())), 1);

			Function *ctor = as<Function>(parent->find(values, scope));
			if (!ctor)
				throw SyntaxError(pos, L"No constructor (" + ::toS(values) + L") found in " + ::toS(parent->identifier()));

			Array<code::Operand> *actuals = new (this) Array<code::Operand>();
			actuals->reserve(values->params->count());

			if (hiddenThread) {
				actuals->push(params->code(0, s, ctor->params->at(0), scope));
				actuals->push(rootBlock->threadParam->var.v);
				for (nat i = 2; i < values->params->count(); i++)
					actuals->push(params->code(i - 1, s, ctor->params->at(i), scope));
			} else {
				for (nat i = 0; i < values->params->count(); i++)
					actuals->push(params->code(i, s, ctor->params->at(i), scope));
			}

			CodeResult *t = CREATE(CodeResult, this);
			ctor->localCall(s, actuals, t, false);
		}

		void SuperCall::callTObject(CodeGen *s, NamedThread *t) {
			if (params->expressions->count() != 1)
				throw SyntaxError(pos, L"Can not initialize a threaded object with parameters.");

			// Find the constructor of TObject.
			Type *parent = TObject::stormType(engine());
			Array<Value> *values = new (this) Array<Value>(2, Value(parent));
			values->at(1) = Value(Thread::stormType(engine()));
			Function *ctor = as<Function>(parent->find(Type::CTOR, values, scope));
			if (!ctor)
				throw InternalError(L"The constructor of TObject: __ctor(TObject, Thread) was not found!");

			// Call the constructor.
			Array<code::Operand> *actuals = new (this) Array<code::Operand>();
			actuals->push(params->code(0, s, ctor->params->at(0), scope));
			actuals->push(t->ref());

			CodeResult *res = new (this) CodeResult();
			ctor->localCall(s, actuals, res, false);
		}

		void SuperCall::code(CodeGen *s, CodeResult *r) {
			using namespace code;

			// Super class should be called first.
			callParent(s);

			code::Var dest = thisVar->var.v;
			Type *type = thisPtr.type;

			// Block for member variable cleanups.
			code::Block varBlock = s->l->createBlock(s->l->last(s->block));
			*s->l << begin(varBlock);

			// From here on, we should set up the destructor to clear 'this' by calling its destructor
			// directly.
			if (Type *super = type->super()) {
				if (Function *fn = super->destructor()) {
					code::Var thisCleanup = s->l->createVar(varBlock, Size::sPtr, fn->directRef(), freeOnException);
					*s->l << mov(thisCleanup, dest);
				}
			}

			// Initialize any member variables.
			Array<MemberVar *> *vars = type->variables();
			for (nat i = 0; i < vars->count(); i++) {
				MemberVar *var = vars->at(i);
				CodeGen *child = s->child(varBlock);
				initVar(child, var);

				Value type = var->type;
				if (type.isValue()) {
					// If we get an exception, this variable should be cleared from here on.
					Part part = s->l->createPart(varBlock);
					*s->l << begin(part);

					code::Var v = s->l->createVar(part, Size::sPtr, type.destructor(), freeOnException);
					*s->l << mov(v, dest);
					*s->l << add(v, ptrConst(var->offset()));
				}
			}

			// Remove all destructors here, since we can call the real destructor of 'this' now.
			*s->l << end(varBlock);

			// Set our VTable.
			if (type->typeFlags & typeClass) {
				type->vtable()->insert(s->l, dest);
			}

			// From here on, we need to make sure that we're freeing our 'this' pointer properly.
			if (Function *fn = type->destructor()) {
				Part p = s->l->createPart(s->block);
				*s->l << begin(p);
				code::Var thisCleanup = s->l->createVar(p, Size::sPtr, fn->directRef(), freeOnException);
				*s->l << mov(thisCleanup, dest);
			}
		}

		void SuperCall::initVar(CodeGen *s, MemberVar *v) {
			InitMap::Iter i = initMap->find(v->name);
			if (i == initMap->end())
				initVarDefault(s, v);
			else
				initVar(s, v, i.v());
		}

		void SuperCall::initVarDefault(CodeGen *s, MemberVar *v) {
			using namespace code;

			Value t = v->type;
			code::Var dest = thisVar->var.v;

			if (t.ref)
				throw SyntaxError(pos, L"Can not initialize reference " + ::toS(v->name) + L", not implemented yet!");

			if (t.isValue()) {
				*s->l << mov(ptrA, dest);
				*s->l << add(ptrA, ptrConst(v->offset()));
				*s->l << fnParam(engine().ptrDesc(), ptrA);
				Function *c = t.type->defaultCtor();
				assert(c, L"No default constructor!");
				*s->l << fnCall(c->ref(), false);
			} else if (t.isBuiltIn()) {
				// Default value is already there.
			} else {
				Function *ctor = t.type->defaultCtor();
				if (!ctor)
					throw SyntaxError(pos, L"Can not initialize " + ::toS(v->name) + L" by default-constructing it. "
									L"Please initialize this member explicitly in " + ::toS(thisPtr) + L".");
				Actuals *params = new (this) Actuals();
				CtorCall *ctorCall = new (this) CtorCall(pos, scope, ctor, params);
				CodeResult *created = new (this) CodeResult(t, s->block);
				ctorCall->code(s, created);

				code::Var cVar = created->location(s).v;
				*s->l << mov(ptrA, dest);
				*s->l << mov(ptrRel(ptrA, v->offset()), cVar);
			}
		}

		void SuperCall::initVar(CodeGen *s, MemberVar *v, Initializer *to) {
			using namespace code;

			Value t = v->type;

			if (t.ref)
				throw SyntaxError(pos, L"Can not initialize reference " + ::toS(v->name) + L", not implemented yet!");

			assert(to->expr || to->params);

			if (to->expr) {
				Expr *init = castTo(to->expr, t, scope);
				if (t.isHeapObj() && init) {
					initVarAssign(s, v, init);
				} else {
					Actuals *p = new (this) Actuals();
					p->add(to->expr);
					initVarCtor(s, v, p);
				}
			} else {
				initVarCtor(s, v, to->params);
			}

		}

		void SuperCall::initVarCtor(CodeGen *s, MemberVar *v, Actuals *to) {
			using namespace code;

			Value t = v->type;
			code::Var dest = thisVar->var.v;
			Type *toCreate = t.type;

			BSNamePart *values = new (this) BSNamePart(Type::CTOR, pos, to);
			values->insert(storm::thisPtr(toCreate));

			Function *ctor = as<Function>(toCreate->find(values, scope));
			if (!ctor)
				throw SyntaxError(pos, L"No constructor for " + ::toS(t) + L"(" + ::toS(values) + L").");

			if (t.isHeapObj()) {
				// Easy way, call the constructor as normal.
				CtorCall *call = new (this) CtorCall(pos, scope, ctor, to);
				CodeResult *created = new (this) CodeResult(t, s->block);
				call->code(s, created);
				VarInfo loc = created->location(s);
				*s->l << mov(ptrA, dest);
				*s->l << mov(ptrRel(ptrA, v->offset()), loc.v);
				loc.created(s);
			} else {
				// Now we're left with the values!

				Array<code::Operand> *actuals = new (this) Array<code::Operand>();
				actuals->reserve(values->params->count());
				// We will prepare 'ptrA' later.
				actuals->push(ptrA);
				for (nat i = 1; i < values->params->count(); i++)
					actuals->push(to->code(i - 1, s, ctor->params->at(i), scope));

				*s->l << mov(ptrA, dest);
				*s->l << add(ptrA, ptrConst(v->offset()));

				CodeResult *nothing = CREATE(CodeResult, this);
				ctor->localCall(s, actuals, nothing, true);
			}
		}

		void SuperCall::initVarAssign(CodeGen *s, MemberVar *v, Expr *to) {
			using namespace code;

			Value t = v->type;
			code::Var dest = thisVar->var.v;
			assert(t.isHeapObj());

			CodeResult *result = new (this) CodeResult(t, s->block);
			to->code(s, result);
			VarInfo loc = result->location(s);
			*s->l << mov(ptrA, dest);
			*s->l << mov(ptrRel(ptrA, v->offset()), loc.v);
		}

	}
}
