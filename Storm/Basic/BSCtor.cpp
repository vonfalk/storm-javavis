#include "stdafx.h"
#include "Syntax/Parser.h"
#include "Shared/TObject.h"
#include "Engine.h"

#include "BSCtor.h"
#include "BSScope.h"
#include "BSNamed.h"
#include "BSClass.h"
#include "BSAutocast.h"

namespace storm {

	bs::BSCtor::BSCtor(const vector<Value> &values, const vector<String> &names,
				const Scope &scope, Par<syntax::Node> body, const SrcPos &pos)
		: Function(Value(), Type::CTOR, values), scope(scope), body(body), paramNames(names), pos(pos) {

		// If we inherit from a class that does not know which thread to run on, we need a Thread as the
		// first parameter (#1, it is after the this ptr).
		RunOn runOn = values[0].type->runOn();
		needsThread = runOn.state == RunOn::runtime;
		if (needsThread) {
			Value expected = Value::thisPtr(Thread::stormType(engine()));

			if (values.size() < 2)
				throw SyntaxError(pos, L"This constructor needs to take a " + ::toS(expected)
								+ L" as the first parameter since we are threaded.");

			if (!expected.canStore(values[1]))
				throw SyntaxError(pos, L"This constructor needs to take a " + ::toS(expected) + L" as the first"
								L" parameter, not " + ::toS(values[1]));
		}

		// Rename until it is initialized!
		paramNames[0] = L" this";

		Auto<FnPtr<CodeGen *>> ptr = memberWeakPtr(engine(), this, &BSCtor::generateCode);
		setCode(steal(CREATE(LazyCode, this, ptr)));
	}

	code::Variable bs::BSCtor::findThread(Par<CodeGen> s, const Actuals &params) {
		using namespace code;

		RunOn on = runOn();
		if (on.state != RunOn::runtime)
			return Function::findThread(s, params);

		// We know it is always the first parameter!
		Variable r = s->frame.createPtrVar(s->block.v);
		s->to << mov(r, params[1]);
		return r;
	}

	bs::CtorBody *bs::BSCtor::parse() {
		if (!body)
			return defaultParse();

		return syntax::transformNode<CtorBody, BSCtor>(body, this);
	}

	bs::CtorBody *bs::BSCtor::defaultParse() {
		Auto<CtorBody> r = CREATE(CtorBody, this, this);
		Auto<Actual> actual = CREATE(Actual, this);
		Auto<SuperCall> super = CREATE(SuperCall, this, pos, r, actual);
		super->pos = pos;
		r->add(super);
		return r.ret();
	}

	CodeGen *bs::BSCtor::generateCode() {
		Auto<CtorBody> body = parse();

		using namespace code;
		Auto<CodeGen> state = CREATE(CodeGen, this, runOn());
		Listing &l = state->to;

		l << prolog();

		// Parameters (first one is special).
		{
			Variable thisVar = l.frame.createParameter(params[0].size(), false);
			Auto<SimplePart> hiddenName = CREATE(SimplePart, this, L" this");
			Auto<SimplePart> normalName = CREATE(SimplePart, this, L"this");
			Auto<LocalVar> hidden = body->variable(hiddenName);
			Auto<LocalVar> normal = body->variable(normalName);
			hidden->var = VarInfo(thisVar);
			normal->var = VarInfo(thisVar);
		}

		for (nat i = 1; i < params.size(); i++) {
			const Value &t = params[i];
			Auto<SimplePart> name = CREATE(SimplePart, this, paramNames[i]);
			Auto<LocalVar> var = body->variable(name);
			assert(var);
			var->createParam(state);
		}

		Auto<CodeResult> r = CREATE(CodeResult, this);
		body->code(state, r);

		l << epilog();
		l << ret(retVoid());

		// PLN(identifier() << L": " << l);
		return state.ret();
	}

	bs::LocalVar *bs::BSCtor::addParams(Par<Block> to) {
		Auto<LocalVar> thread;

		for (nat i = 0; i < params.size(); i++) {
			Auto<LocalVar> var = CREATE(LocalVar, this, paramNames[i], params[i], pos, true);
			if (i == 0)
				var->constant = true;
			to->add(var);
			if (i == 1 && needsThread)
				thread = var;
		}

		return thread.ret();
	}

	bs::CtorBody::CtorBody(Par<BSCtor> ctor) : ExprBlock(ctor->pos, ctor->scope) {
		threadParam = ctor->addParams(this);
	}

	void bs::CtorBody::add(Par<ArrayP<Expr>> exprs) {
		for (nat i = 0; i < exprs->count(); i++) {
			add(exprs->at(i));
		}
	}

	void bs::CtorBody::blockCode(Par<CodeGen> state, Par<CodeResult> to, const code::Block &block) {
		if (threadParam) {
			thread = state->frame.createPtrVar(state->block.v);
			state->to << mov(thread, threadParam->var.var());
		}

		Block::blockCode(state, to, block);
	}

	bs::Initializer::Initializer(Par<SStr> name, Par<Expr> expr) : name(name), expr(expr) {}

	bs::Initializer::Initializer(Par<SStr> name, Par<Actual> params) : name(name), params(params) {}

	bs::SuperCall::SuperCall(SrcPos pos, Par<CtorBody> block, Par<Actual> params, Par<ArrayP<Initializer>> init)
		: Expr(pos) {

		this->init(block, params);
		for (nat i = 0; i < init->count(); i++)
			this->init(init->at(i));
	}

	bs::SuperCall::SuperCall(SrcPos pos, Par<CtorBody> block, Par<Actual> params) : Expr(pos) {
		init(block, params);
	}

	void bs::SuperCall::init(Par<CtorBody> block, Par<Actual> params) {
		rootBlock = block.borrow();

		// Add the regular this parameter!
		Auto<SimplePart> name = CREATE(SimplePart, this, L" this");
		thisVar = steal(block->variable(name));
		thisPtr = thisVar->result;
		Auto<LocalVar> created = CREATE(LocalVar, this, L"this", thisPtr, thisVar->pos, true);
		created->constant = true;
		block->add(created);

		this->params = params;
		Auto<LocalVarAccess> l = CREATE(LocalVarAccess, this, pos, thisVar);
		params->addFirst(l);

		scope = block->scope;
	}

	ExprResult bs::SuperCall::result() {
		return ExprResult();
	}

	void bs::SuperCall::init(Par<Initializer> init) {
		const String &name = init->name->v->v;
		Auto<TypeVar> v = steal(thisPtr.type->findCpp(name, vector<Value>(1, thisPtr))).as<TypeVar>();
		if (v == null || v->params[0].type != thisPtr.type)
			throw SyntaxError(init->name->pos, L"The member variable " + name + L" was not found in "
							+ ::toS(thisPtr));

		if (initMap.count(name))
			throw SyntaxError(init->name->pos, L"The member " + name + L" has already been initialized.");

		initMap.insert(make_pair(name, init));
	}

	void bs::SuperCall::callParent(Par<CodeGen> s) {
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
		Auto<BSNamePart> values = CREATE(BSNamePart, this, Type::CTOR, pos, params);
		values->alter(0, parent);

		if (hiddenThread)
			values->insert(Value::thisPtr(Thread::stormType(engine())), 1);

		Auto<Function> ctor = steal(parent->find(values)).as<Function>();
		if (!ctor)
			throw SyntaxError(pos, L"No constructor (" + ::toS(values) + L") found in " + parent->identifier());

		vector<code::Value> actuals;
		actuals.reserve(values->count());

		if (hiddenThread) {
			actuals.push_back(params->code(0, s, ctor->params[0]));
			actuals.push_back(rootBlock->threadParam->var.var());
			for (nat i = 2; i < values->count(); i++)
				actuals.push_back(params->code(i - 1, s, ctor->params[i]));
		} else {
			for (nat i = 0; i < values->count(); i++)
				actuals.push_back(params->code(i, s, ctor->params[i]));
		}

		Auto<CodeResult> t = CREATE(CodeResult, this);
		ctor->localCall(s, actuals, t, false);
	}

	void bs::SuperCall::callTObject(Par<CodeGen> s, Par<NamedThread> t) {
		if (params->expressions.size() != 1)
			throw SyntaxError(pos, L"Can not initialize a threaded object with parameters.");

		// Find the constructor of TObject.
		Type *parent = TObject::stormType(engine());
		vector<Value> values = valList(2, Value(parent), Value(Thread::stormType(engine())));
		Auto<Function> ctor = steal(parent->findCpp(Type::CTOR, values)).as<Function>();
		if (!ctor)
			throw InternalError(L"The constructor of TObject: __ctor(TObject, Thread) was not found!");

		// Call the constructor.
		vector<code::Value> actuals(2);
		actuals[0] = params->code(0, s, ctor->params[0]);
		actuals[1] = t->ref();

		Auto<CodeResult> res = CREATE(CodeResult, this);
		ctor->localCall(s, actuals, res, false);
	}

	void bs::SuperCall::code(Par<CodeGen> s, Par<CodeResult> r) {
		using namespace code;

		// Super class should be called first.
		callParent(s);

		Variable dest = thisVar->var.var();
		Type *type = thisPtr.type;

		// Block for member variable cleanups.
		code::Block varBlock = s->frame.createChildLast(s->block.v);
		s->to << begin(varBlock);

		// From here on, we should set up the destructor to clear 'this' by calling its destructor
		// directly.
		if (Type *super = type->super()) {
			if (Function *fn = super->destructor()) {
				Variable thisCleanup = s->frame.createPtrVar(varBlock, fn->directRef(), freeOnException);
				s->to << mov(thisCleanup, dest);
			}
		}

		// Initialize any member variables.
		vector<Auto<TypeVar>> vars = type->variables();
		for (nat i = 0; i < vars.size(); i++) {
			Auto<TypeVar> var = vars[i];
			Auto<CodeGen> child = s->child(varBlock);
			initVar(child, var);

			// If we get an exception, this variable should be cleared from here on.
			Part part = s->frame.createPart(varBlock);
			s->to << begin(part);

			Value type = var->varType;
			if (type.isClass()) {
				Variable v = s->frame.createPtrVar(part, engine().fnRefs.release, freeOnException);
				s->to << mov(ptrA, dest);
				s->to << mov(v, ptrRel(ptrA, var->offset()));
			} else {
				Variable v = s->frame.createPtrVar(part, type.destructor(), freeOnException);
				s->to << mov(v, dest);
				s->to << add(v, intPtrConst(var->offset()));
			}
		}

		// Remove all destructors here, since we can call the real destructor of 'this' now.
		s->to << end(varBlock);

		// Set our VTable.
		if (type->typeFlags & typeClass) {
			// TODO: maybe symbolic offset here?
			s->to << mov(ptrA, dest);
			s->to << mov(ptrRel(ptrA), type->vtable.ref);
		}

		// From here on, we need to make sure that we're freeing our 'this' pointer properly.
		if (Function *fn = type->destructor()) {
			Part p = s->frame.createPart(s->block.v);
			s->to << begin(p);
			Variable thisCleanup = s->frame.createPtrVar(p, fn->directRef(), freeOnException);
			s->to << mov(thisCleanup, dest);
		}
	}

	void bs::SuperCall::initVar(Par<CodeGen> s, Par<TypeVar> v) {
		InitMap::iterator i = initMap.find(v->name);
		if (i == initMap.end())
			initVarDefault(s, v);
		else
			initVar(s, v, i->second);
	}

	void bs::SuperCall::initVarDefault(Par<CodeGen> s, Par<TypeVar> v) {
		using namespace code;

		const Value &t = v->varType;
		Variable dest = thisVar->var.var();

		if (t.ref)
			throw SyntaxError(pos, L"Can not initialize reference " + v->name + L", not implemented yet!");

		if (t.isValue()) {
			s->to << mov(ptrA, dest);
			s->to << add(ptrA, intPtrConst(v->offset()));
			s->to << fnParam(ptrA);
			s->to << fnCall(t.defaultCtor(), retVoid());
		} else if (t.isBuiltIn()) {
			// Default value is already there.
		} else {
			Function *ctor = t.type->defaultCtor();
			if (!ctor)
				throw SyntaxError(pos, L"Can not initialize " + v->name + L" by default-constructing it. In "
								+ ::toS(thisPtr) + L", please initialize this member explicitly.");
			Auto<Actual> params = CREATE(Actual, this);
			Auto<CtorCall> ctorCall = CREATE(CtorCall, this, pos, ctor, params);
			Auto<CodeResult> created = CREATE(CodeResult, this, t, s->block);
			ctorCall->code(s, created);

			code::Variable cVar = created->location(s).var();
			s->to << mov(ptrA, dest);
			s->to << mov(ptrRel(ptrA, v->offset()), cVar);
			s->to << code::addRef(cVar);
		}
	}

	void bs::SuperCall::initVar(Par<CodeGen> s, Par<TypeVar> v, Par<Initializer> to) {
		using namespace code;

		const Value &t = v->varType;

		if (t.ref)
			throw SyntaxError(pos, L"Can not initialize reference " + v->name + L", not implemented yet!");

		assert(to->expr || to->params);

		if (to->expr) {
			Auto<Expr> init = castTo(to->expr, t);
			if (t.isClass() && init) {
				initVarAssign(s, v, init);
			} else {
				Auto<Actual> p = CREATE(Actual, this);
				p->add(to->expr);
				initVarCtor(s, v, p);
			}
		} else {
			initVarCtor(s, v, to->params);
		}

	}

	void bs::SuperCall::initVarCtor(Par<CodeGen> s, Par<TypeVar> v, Par<Actual> to) {
		using namespace code;

		const Value &t = v->varType;
		Variable dest = thisVar->var.var();
		Type *toCreate = t.type;

		Auto<BSNamePart> values = CREATE(BSNamePart, this, Type::CTOR, pos, to);
		values->insert(Value::thisPtr(toCreate));

		Auto<Function> ctor = steal(toCreate->find(values)).as<Function>();
		if (!ctor)
			throw SyntaxError(pos, L"No constructor for " + ::toS(t) + L"(" + ::toS(values) + L").");

		if (t.isClass()) {
			// Easy way, call the constructor as normal.
			Auto<CtorCall> call = CREATE(CtorCall, this, pos, ctor, to);
			Auto<CodeResult> created = CREATE(CodeResult, this, t, s->block);
			call->code(s, created);
			VarInfo loc = created->location(s);
			s->to << mov(ptrA, dest);
			s->to << mov(ptrRel(ptrA, v->offset()), loc.var());
			s->to << code::addRef(loc.var());
			loc.created(s);
		} else {
			// Now we're left with the values!

			vector<code::Value> actuals(values->count());
			for (nat i = 1; i < values->count(); i++)
				actuals[i] = to->code(i - 1, s, ctor->params[i]);

			s->to << mov(ptrA, dest);
			s->to << add(ptrA, intPtrConst(v->offset()));
			actuals[0] = ptrA;

			Auto<CodeResult> nothing = CREATE(CodeResult, this);
			ctor->localCall(s, actuals, nothing, true);
		}
	}

	void bs::SuperCall::initVarAssign(Par<CodeGen> s, Par<TypeVar> v, Par<Expr> to) {
		using namespace code;

		const Value &t = v->varType;
		Variable dest = thisVar->var.var();
		assert(t.isClass());

		Auto<CodeResult> result = CREATE(CodeResult, this, t, s->block);
		to->code(s, result);
		VarInfo loc = result->location(s);
		s->to << mov(ptrA, dest);
		s->to << mov(ptrRel(ptrA, v->offset()), loc.var());
		s->to << code::addRef(loc.var());
	}

}
