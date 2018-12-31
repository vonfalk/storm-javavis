#include "stdafx.h"
#include "Function.h"
#include "Cast.h"
#include "Doc.h"
#include "Scope.h"
#include "Core/Fn.h"
#include "Compiler/Code.h"
#include "Compiler/Exception.h"
#include "Compiler/Engine.h"
#include "Compiler/Syntax/Parser.h"

namespace storm {
	namespace bs {
		using syntax::SStr;

		FunctionDecl *assignDecl(Scope scope,
								syntax::SStr *name,
								Array<NameParam> *params,
								syntax::Node *options,
								syntax::Node *body) {

			return new (name) FunctionDecl(scope, null, name, params, options, body);
		}



		FunctionDecl::FunctionDecl(Scope scope,
								MAYBE(SrcName *) result,
								syntax::SStr *name,
								Array<NameParam> *params,
								syntax::Node *options,
								syntax::Node *body)
			: scope(scope),
			  name(name),
			  result(result),
			  params(params),
			  options(options),
			  body(body) {}



		Named *FunctionDecl::doCreate() {
			Scope fScope = fileScope(scope, name->pos);

			Value result;
			if (this->result)
				result = fScope.value(this->result);

			// if (*name->v == S("asyncPostObject"))
			// 	PVAR(body);

			Array<ValParam> *par = bs::resolve(params, fScope);
			BSFunction *f = new (this) BSFunction(result, name, par, scope, null, body);

			// Default thread?
			if (thread)
				f->thread(scope, thread);

			if (!this->result)
				f->make(fnAssign);

			if (options)
				syntax::transformNode<void, Scope, BSRawFn *>(options, fScope, f);

			return f;
		}

		MAYBE(Named *) bs::FunctionDecl::update(Scope scope) {
			NamePart *name = namePart();
			Named *found = scope.find(new (this) Name(name));
			if (BSFunction *bs = as<BSFunction>(found)) {
				update(bs);
			} else if (found) {
				// Can not update the function. We don't know it!
			} else {
				Named *c = create();
				resolve();
				return c;
			}

			return null;
		}

		void bs::FunctionDecl::update(BSFunction *fn) {
			Value result;
			if (this->result)
				result = scope.value(this->result);

			assert(fn->result == result);
			fn->update(bs::resolve(params, scope), body, name->pos);
		}

		NamePart *bs::FunctionDecl::namePart() const {
			return new (this) SimplePart(name->v, values(bs::resolve(params, scope)));
		}

		void bs::FunctionDecl::toS(StrBuf *to) const {
			*to << namePart();
		}


		/**
		 * Raw function.
		 */

		BSRawFn::BSRawFn(Value result, SStr *name, Array<ValParam> *params, MAYBE(NamedThread *) thread)
			: Function(result, name->v, values(params)), pos(name->pos), valParams(params) {

			init(thread);
		}

		void BSRawFn::init(NamedThread *thread) {
			if (thread)
				runOn(thread);

			if (result.ref)
				throw SyntaxError(pos, L"Returning references is not a good idea!");

			setCode(new (this) LazyCode(fnPtr(engine(), &BSRawFn::generateCode, this)));
		}

		void BSRawFn::thread(Scope scope, SrcName *name) {
			if (parentLookup)
				throw InternalError(L"Can not call BSRawFn:thread after the function is attached somewhere.");

			Scope fScope = fileScope(scope, name->pos);
			NamedThread *t = as<NamedThread>(fScope.find(name));
			if (!t)
				throw SyntaxError(name->pos, ::toS(name) + L" is not a named thread.");
			runOn(t);
		}

		FnBody *BSRawFn::createBody() {
			throw InternalError(L"A BSRawFn can not be used without overriding 'createBody'!");
		}

		void BSRawFn::makeStatic() {
			if (parentLookup)
				throw InternalError(L"Don't call 'makeStatic' after adding the function to the name tree.");

			// Already static?
			if (fnFlags() & fnStatic)
				return;

			FnFlags outlawed = fnAbstract | fnFinal | fnOverride;
			if (fnFlags() & outlawed)
				throw SyntaxError(pos, L"Can not make functions marked abstract, final or override into static functions.");

			if (valParams->empty())
				return;
			if (*valParams->at(0).name != S("this"))
				return;

			// All seems well, proceed.
			make(fnStatic);
			valParams->remove(0);
			params->remove(0);
		}

		CodeGen *BSRawFn::generateCode() {
			FnBody *body = createBody();

			// Expression possibly wrapped around the body (casting the value if needed).
			Expr *bodyExpr = expectCastTo(body, result, Scope(this));
			// PLN(bodyExpr);

			// Generate code!
			using namespace code;
			CodeGen *state = new (this) CodeGen(runOn(), isMember(), result);

			Listing *l = state->l;

			*l << prolog();

			// Parameters
			for (nat i = 0; i < valParams->count(); i++) {
				SimplePart *name = new (this) SimplePart(valParams->at(i).name);
				LocalVar *var = body->variable(name);
				assert(var);
				var->createParam(state);
			}

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

			// if (!identifier()->startsWith(S("lang.bs"))) {
			// if (identifier()->startsWith(S("test.bs."))) {
			// 	PLN(bodyExpr);
			// 	PLN(identifier() << L": " << l);
			// 	// This can be used to see the transformed machine code as well:
			// 	PLN(engine().arena()->transform(l, null));
			// 	// See transformed machine code and binary output (similar to above).
			// 	// PVAR(new (this) Binary(engine().arena(), l, true));
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
			for (Nat i = 0; i < valParams->count(); i++) {
				LocalVar *v = new (this) LocalVar(valParams->at(i).name, valParams->at(i).type, pos, true);

				if (parentLookup) {
					if (i == 0 && isMember())
						v->constant = true;
				}

				to->add(v);
			}
		}

		Array<DocParam> *BSRawFn::docParams() {
			Array<DocParam> *r = new (this) Array<DocParam>();
			r->reserve(valParams->count());

			for (Nat i = 0; i < valParams->count(); i++)
				r->push(DocParam(valParams->at(i).name, valParams->at(i).type));

			return r;
		}


		/**
		 * Function.
		 */

		BSFunction::BSFunction(Value result, SStr *name, Array<ValParam> *params, Scope scope,
								MAYBE(NamedThread *) thread, syntax::Node *body) :
			BSRawFn(result, name, params, thread), scope(scope), body(body) {}

		bs::FnBody *BSFunction::createBody() {
			if (!body)
				throw InternalError(L"Multiple compilation of a function!");

			FnBody *result = syntax::transformNode<FnBody, BSFunction *>(body, this);
			// We don't need to keep it around anymore.
			body = null;
			return result;
		}

		Bool BSFunction::update(Array<ValParam> *params, syntax::Node *node, SrcPos pos) {
			if (valParams->count() != params->count())
				return false;
			for (Nat i = 0; i < params->count(); i++)
				if (valParams->at(i).type != params->at(i).type)
					return false;

			valParams = params;
			this->body = node;
			this->pos = pos;
			reset();
			return true;
		}

		Bool BSFunction::update(Array<ValParam> *params, syntax::Node *node) {
			return update(params, node, this->pos);
		}

		Bool BSFunction::update(Array<Str *> *params, syntax::Node *body) {
			if (valParams->count() != params->count())
				return false;
			for (Nat i = 0; i < params->count(); i++)
				valParams->at(i).name = params->at(i);

			this->body = body;
			reset();
			return true;
		}

		Bool BSFunction::update(BSFunction *from) {
			return update(from->valParams, from->body);
		}


		/**
		 * Tree function.
		 */

		BSTreeFn::BSTreeFn(Value result, SStr *name, Array<ValParam> *params, MAYBE(NamedThread *) thread)
			: BSRawFn(result, name, params, thread) {}

		void BSTreeFn::body(FnBody *body) {
			root = body;
			reset();
		}

		bs::FnBody *BSTreeFn::createBody() {
			if (!root)
				throw RuntimeError(L"The body of " + ::toS(identifier()) + L"was not set before trying to use it.");

			FnBody *result = root;
			root = null;
			return result;
		}


		/**
		 * Abstract function.
		 */

		BSAbstractFn::BSAbstractFn(Value result, SStr *name, Array<ValParam> *params)
			: BSRawFn(result, name, params, null) {

			// Just to make sure...
			makeAbstract();
		}

		FnBody *BSAbstractFn::createBody() {
			// We don't have anything to return, and we don't really care if this is a bit slower if
			// we were to actually generate machine code for throwing the exception:
			throw AbstractFnCalled(::toS(identifier()));
		}


		/**
		 * Function body.
		 */

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
