#include "stdafx.h"
#include "Function.h"
#include "Cast.h"
#include "Core/Fn.h"
#include "Compiler/Code.h"
#include "Compiler/Exception.h"
#include "Compiler/Engine.h"
#include "Compiler/Syntax/Parser.h"

namespace storm {
	namespace bs {
		using syntax::SStr;

		FunctionDecl::FunctionDecl(Scope scope,
								SrcName *result,
								syntax::SStr *name,
								Array<NameParam> *params,
								syntax::Node *body)
			: scope(scope),
			  name(name),
			  result(result),
			  params(params),
			  thread(null),
			  body(body) {}

		FunctionDecl::FunctionDecl(Scope scope,
								SrcName *result,
								syntax::SStr *name,
								Array<NameParam> *params,
								SrcName *thread,
								syntax::Node *body)
			: scope(scope),
			  name(name),
			  result(result),
			  params(params),
			  thread(thread),
			  body(body) {}

		Function *FunctionDecl::createFn() {
			Value result = scope.value(this->result);
			NamedThread *thread = null;

			if (this->thread) {
				if (NamedThread *t = as<NamedThread>(scope.find(this->thread))) {
					thread = t;
				} else {
					throw SyntaxError(this->thread->pos, ::toS(this->thread) + L" is not a thread.");
				}
			}

			// if (wcscmp(name->v->c_str(), L"asyncPostObject") == 0)
			// 	PVAR(body);

			return new (this) BSFunction(result, name, resolve(params, scope), scope, thread, body);
		}

		void bs::FunctionDecl::update(BSFunction *fn) {
			Value result = scope.value(this->result);

			assert(fn->result == result);
			fn->update(resolve(params, scope), body, name->pos);
		}

		NamePart *bs::FunctionDecl::namePart() const {
			return new (this) SimplePart(name->v, values(resolve(params, scope)));
		}


		BSRawFn::BSRawFn(Value result, SStr *name, Array<ValParam> *params, MAYBE(NamedThread *) thread)
			: Function(result, name->v, values(params)), pos(name->pos), params(params) {

			init(thread);
		}

		void BSRawFn::init(NamedThread *thread) {
			if (thread)
				runOn(thread);

			if (result.ref)
				throw SyntaxError(pos, L"Returning references is not a good idea!");

			setCode(new (this) LazyCode(fnPtr(engine(), &BSRawFn::generateCode, this)));
		}

		FnBody *BSRawFn::createBody() {
			throw InternalError(L"A BSRawFn can not be used without overriding 'createBody'!");
		}

		CodeGen *BSRawFn::generateCode() {
			FnBody *body = createBody();

			// Expression possibly wrapped around the body (casting the value if needed).
			Expr *bodyExpr = expectCastTo(body, result);
			// PLN(bodyExpr);

			// Generate code!
			using namespace code;
			CodeGen *state = new (this) CodeGen(runOn());

			Listing *l = state->l;

			*l << prolog();

			// Parameters
			for (nat i = 0; i < params->count(); i++) {
				SimplePart *name = new (this) SimplePart(params->at(i).name);
				LocalVar *var = body->variable(name);
				assert(var);
				var->createParam(state);
			}

			// Return type.
			state->result(result, isMember());

			if (result == Value()) {
				CodeResult *r = CREATE(CodeResult, this);
				bodyExpr->code(state, r);
				state->returnValue(code::Var());
			} else {
				// TODO? Do we need to check if 'r' is a reference first?
				CodeResult *r = new (this) CodeResult(result, l->root());
				bodyExpr->code(state, r);

				VarInfo rval = r->location(state);
				state->returnValue(rval.v);
			}

			// if (!identifier()->startsWith(L"lang.bs")) {
			// 	PLN(bodyExpr);
			// 	PLN(identifier() << L": " << l);
			// 	// This can be used to see the transformed machine code as well:
			// 	// PLN(engine().arena()->transform(l, null));
			// }

			// PLN(bodyExpr);
			// PLN(identifier() << L": " << l);
			return state;
		}

		void BSRawFn::reset() {
			// Could be done better...
			setCode(new (this) LazyCode(fnPtr(engine(), &BSRawFn::generateCode, this)));
		}

		void BSRawFn::addParams(Block *to) {
			for (nat i = 0; i < params->count(); i++) {
				LocalVar *v = new (this) LocalVar(params->at(i).name, params->at(i).type, pos, true);

				if (parentLookup) {
					if (i == 0 && isMember())
						v->constant = true;
				}

				to->add(v);
			}
		}


		BSFunction::BSFunction(Value result, SStr *name, Array<ValParam> *params, Scope scope,
								MAYBE(NamedThread *) thread, syntax::Node *body) :
			BSRawFn(result, name, params, thread), scope(scope), body(body) {}

		bs::FnBody *BSFunction::createBody() {
			return syntax::transformNode<FnBody, BSFunction *>(body, this);
		}

		Bool BSFunction::update(Array<ValParam> *params, syntax::Node *node, SrcPos pos) {
			if (this->params->count() != params->count())
				return false;
			for (Nat i = 0; i < params->count(); i++)
				if (this->params->at(i).type != params->at(i).type)
					return false;

			this->params = params;
			this->body = node;
			this->pos = pos;
			reset();
			return true;
		}

		Bool BSFunction::update(Array<ValParam> *params, syntax::Node *node) {
			return update(params, node, this->pos);
		}

		Bool BSFunction::update(Array<Str *> *params, syntax::Node *body) {
			if (this->params->count() != params->count())
				return false;
			for (Nat i = 0; i < params->count(); i++)
				this->params->at(i).name = params->at(i);

			this->body = body;
			reset();
			return true;
		}

		Bool BSFunction::update(BSFunction *from) {
			return update(from->params, from->body);
		}

		BSTreeFn::BSTreeFn(Value result, SStr *name, Array<ValParam> *params, MAYBE(NamedThread *) thread)
			: BSRawFn(result, name, params, thread) {}

		void BSTreeFn::body(FnBody *body) {
			if (root)
				reset();

			root = body;
		}

		bs::FnBody *BSTreeFn::createBody() {
			if (!root)
				throw RuntimeError(L"The body of " + ::toS(identifier()) + L"was not set before trying to use it.");

			return root;
		}



		bs::FnBody::FnBody(BSRawFn *owner, Scope scope)
			: ExprBlock(owner->pos, scope), type(owner->result) {
		 	owner->addParams(this);
		}

		bs::FnBody::FnBody(BSFunction *owner)
			: ExprBlock(owner->pos, owner->scope), type(owner->result) {
			owner->addParams(this);
		}

	}
}
