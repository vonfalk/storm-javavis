#include "stdafx.h"
#include "BSFunction.h"
#include "BSAutocast.h"
#include "Code/Instruction.h"
#include "Syntax/Parser.h"
#include "PkgReader.h"
#include "Exception.h"
#include "Thread.h"
#include "NamedThread.h"

namespace storm {

	bs::FunctionDecl::FunctionDecl(Par<SyntaxEnv> env,
								Par<SrcName> result,
								Par<SStr> name,
								Par<Params> params,
								Par<syntax::Node> body) :
		env(env), name(name), result(result), params(params), thread(null), body(body) {}

	bs::FunctionDecl::FunctionDecl(Par<SyntaxEnv> env,
								Par<SrcName> result,
								Par<SStr> name,
								Par<Params> params,
								Par<SrcName> thread,
								Par<syntax::Node> body) :
		env(env), name(name), result(result), params(params), thread(thread), body(body) {}


	Function *bs::FunctionDecl::createFn() {
		const Scope &scope = env->scope;

		Value result = scope.value(this->result);
		NamedThread *thread = null;

		if (this->thread) {
			Auto<Named> n = scope.find(this->thread);
			if (NamedThread *t = as<NamedThread>(n.borrow())) {
				thread = t;
			} else {
				throw SyntaxError(this->thread->pos, L"The identifier " + ::toS(thread) + L" is not a thread.");
			}
		}

		return CREATE(BSFunction, this, result, name, params, scope, thread, body);
	}

	void bs::FunctionDecl::update(Par<BSFunction> fn) {
		static bool first = true;
		if (first) {
			WARNING(L"This is a hack, and should be solved better later!");
			first = false;
		}

		const Scope &scope = env->scope;
		Value result = scope.value(this->result);
		vector<Value> params = this->params->cTypes(scope);
		vector<String> names = this->params->cNames();

		assert(fn->result == result);
		assert(params.size() == fn->params.size());
		for (nat i = 0; i < params.size(); i++)
			assert(params[i] == fn->params[i]);

		fn->update(names, body, name->pos);
	}

	NamePart *bs::FunctionDecl::namePart() const {
		return CREATE(SimplePart, this, name->v->v, params->cTypes(env->scope));
	}

	bs::BSRawFn::BSRawFn(Value result, Par<SStr> name, Par<Array<Value>> params, Par<ArrayP<Str>> names,
						MAYBE(Par<NamedThread>) thread) :
		Function(result, name->v, params), pos(name->pos) {

		for (nat i = 0; i < names->count(); i++) {
			paramNames.push_back(names->at(i)->v);
		}

		init(thread);
	}

	bs::BSRawFn::BSRawFn(Value result, Par<SStr> name, const vector<Value> &params, const vector<String> &names,
						MAYBE(Par<NamedThread>) thread) :
		Function(result, name->v->v, params), paramNames(names), pos(name->pos) {

		init(thread);
	}

	void bs::BSRawFn::init(Par<NamedThread> thread) {
		if (thread)
			runOn(thread);

		if (result.ref)
			throw SyntaxError(pos, L"Returning references is not a good idea at this point!");

		if (paramNames.size() != params.size())
			throw SyntaxError(pos, L"The number of parameter names does not match up to the number of parameter types.");

		Auto<FnPtr<CodeGen *>> fn = memberWeakPtr(engine(), this, &BSRawFn::generateCode);
		setCode(steal(CREATE(LazyCode, this, fn)));
	}

	bs::FnBody *bs::BSRawFn::createBody() {
		throw InternalError(L"A BSRawFn can not be used without overriding 'createBody'!");
	}

	CodeGen *bs::BSRawFn::generateCode() {
		Auto<FnBody> body = createBody();

		// Expression possibly wrapped around the body (casting the value if needed).
		Auto<Expr> bodyExpr = expectCastTo(body, result);

		// Generate code!
		using namespace code;
		Auto<CodeGen> state = CREATE(CodeGen, this, runOn());

		Listing &l = state->to;

		l << prolog();

		// Parameters
		for (nat i = 0; i < params.size(); i++) {
			Auto<SimplePart> name = CREATE(SimplePart, this, paramNames[i]);
			Auto<LocalVar> var = body->variable(name);
			assert(var);
			var->createParam(state);
		}

		// Return type.
		state->returnType(result, isMember());

		if (result == Value()) {
			Auto<CodeResult> r = CREATE(CodeResult, this);
			bodyExpr->code(state, r);
			state->returnValue(wrap::Variable());
		} else {
			// TODO? Do we need to check if 'r' is a reference first?
			Auto<CodeResult> r = CREATE(CodeResult, this, result, l.frame.root());
			bodyExpr->code(state, r);

			VarInfo rval = r->location(state);
			state->returnValue(rval.var());
		}

		// if (!identifier().startsWith(L"lang.bs")) {
		// 	PLN(bodyExpr);
		// 	PLN(identifier() << L": " << l);
		// }

		return state.ret();
	}

	void bs::BSRawFn::reset() {
		// Could be done better...
		Auto<FnPtr<CodeGen *>> fn = memberWeakPtr(engine(), this, &BSRawFn::generateCode);
		setCode(steal(CREATE(LazyCode, this, fn)));
	}

	void bs::BSRawFn::addParams(Par<Block> to) {
		for (nat i = 0; i < params.size(); i++) {
			Auto<LocalVar> var = CREATE(LocalVar, this, paramNames[i], params[i], pos, true);

			// TODO: We do not want this if-statement!
			if (parentLookup) {
				if (i == 0 && isMember())
					var->constant = true;
			}
			to->add(var);
		}
	}


	bs::BSFunction::BSFunction(Value result, Par<SStr> name, Par<Params> params, Scope scope,
							MAYBE(Par<NamedThread>) thread, Par<syntax::Node> body) :
		BSRawFn(result, name, params->cTypes(scope), params->cNames(), thread), scope(scope), body(body) {}

	void bs::BSFunction::update(const vector<String> &names, Par<syntax::Node> body, const SrcPos &pos) {
		paramNames = names;
		this->body = body;
		this->pos = pos;

		reset();
	}

	Bool bs::BSFunction::update(Par<ArrayP<Str>> names, Par<syntax::Node> body) {
		if (names->count() != params.size())
			return false;

		vector<String> n(names->count());
		for (nat i = 0; i < names->count(); i++) {
			n[i] = names->at(i)->v;
		}

		update(n, body, pos);

		return true;
	}

	void bs::BSFunction::update(Par<BSFunction> from) {
		assert(paramNames.size() == from->paramNames.size());
		paramNames = from->paramNames;
		body = from->body;
		pos = from->pos;

		reset();
	}

	bs::FnBody *bs::BSFunction::createBody() {
		return syntax::transformNode<FnBody, BSFunction>(body, this);
	}

	bs::BSTreeFn::BSTreeFn(Value result, Par<SStr> name, Par<Array<Value>> params,
						Par<ArrayP<Str>> names, MAYBE(Par<NamedThread>) thread) :
		BSRawFn(result, name, params, names, thread) {
	}

	void bs::BSTreeFn::body(Par<FnBody> body) {
		if (root)
			reset();

		root = body;
	}

	bs::FnBody *bs::BSTreeFn::createBody() {
		if (!root)
			throw RuntimeError(L"The body of " + identifier() + L"was not set before trying to use it.");

		return root.ret();
	}



	bs::FnBody::FnBody(Par<BSRawFn> owner, Scope scope) : ExprBlock(owner->pos, scope), type(owner->result) {
		owner->addParams(this);
	}

	bs::FnBody::FnBody(Par<BSFunction> owner) : ExprBlock(owner->pos, owner->scope), type(owner->result) {
		owner->addParams(this);
	}

}