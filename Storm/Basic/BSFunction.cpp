#include "stdafx.h"
#include "BSFunction.h"
#include "BSAutocast.h"
#include "Code/Instruction.h"
#include "Parser.h"
#include "PkgReader.h"
#include "Exception.h"
#include "Thread.h"
#include "NamedThread.h"

namespace storm {

	bs::FunctionDecl::FunctionDecl(SrcPos pos,
								Par<SStr> name,
								Par<TypeName> result,
								Par<Params> params,
								Par<SStr> contents)
		: pos(pos), name(name), result(result), params(params), thread(null), contents(contents) {}

	bs::FunctionDecl::FunctionDecl(SrcPos pos,
								Par<SStr> name,
								Par<TypeName> result,
								Par<Params> params,
								Par<TypeName> thread,
								Par<SStr> contents)
		: pos(pos), name(name), result(result), params(params), thread(thread), contents(contents) {}

	Function *bs::FunctionDecl::asFunction(const Scope &scope) {
		Value result = this->result->resolve(scope);
		NamedThread *thread = null;

		if (this->thread) {
			Auto<Named> n = this->thread->find(scope);
			if (NamedThread *t = as<NamedThread>(n.borrow())) {
				thread = t;
			} else {
				throw SyntaxError(this->thread->pos, L"The identifier " + ::toS(thread) + L" is not a thread.");
			}
		}

		return CREATE(BSFunction, this, result, name, params, scope, thread, contents);
	}

	void bs::FunctionDecl::update(Par<BSFunction> fn, const Scope &scope) {
		static bool first = true;
		if (first) {
			WARNING(L"This is a hack, and should be solved better later!");
			first = false;
		}

		Value result = this->result->resolve(scope);
		vector<Value> params = this->params->cTypes(scope);
		vector<String> names = this->params->cNames();

		assert(fn->result == result);
		assert(params.size() == fn->params.size());
		for (nat i = 0; i < params.size(); i++)
			assert(params[i] == fn->params[i]);

		fn->update(names, contents, name->pos);
	}

	NamePart *bs::FunctionDecl::namePart(const Scope &scope) const {
		return CREATE(NamePart, this, name->v->v, params->cTypes(scope));
	}


	bs::BSRawFn::BSRawFn(Value result, Par<SStr> name, Par<Params> params,
						Scope scope, MAYBE(Par<NamedThread>) thread) :
		Function(result, name->v->v, params->cTypes(scope)), scope(scope), paramNames(params->cNames()), pos(name->pos) {

		if (thread)
			runOn(thread);

		if (result.ref)
			throw SyntaxError(pos, L"Returning references is not a good idea at this point!");

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
			Auto<NamePart> name = CREATE(NamePart, this, paramNames[i]);
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
			if (i == 0 && isMember())
				var->constant = true;
			to->add(var);
		}
	}


	bs::BSFunction::BSFunction(Value result, Par<SStr> name, Par<Params> params, Scope scope,
							MAYBE(Par<NamedThread>) thread, Par<SStr> contents) :
		BSRawFn(result, name, params, scope, thread), contents(contents) {}

	void bs::BSFunction::update(const vector<String> &names, Par<SStr> contents, const SrcPos &pos) {
		paramNames = names;
		this->contents = contents;
		this->pos = pos;

		reset();
	}

	Bool bs::BSFunction::update(Par<ArrayP<Str>> names, Par<SStr> contents) {
		if (names->count() != params.size())
			return false;

		vector<String> n(names->count());
		for (nat i = 0; i < names->count(); i++) {
			n[i] = names->at(i)->v;
		}

		update(n, contents, pos);

		return true;
	}

	void bs::BSFunction::update(Par<BSFunction> from) {
		assert(paramNames.size() == from->paramNames.size());
		paramNames = from->paramNames;
		contents = from->contents;
		pos = from->pos;

		reset();
	}

	bs::FnBody *bs::BSFunction::createBody() {
		Auto<SyntaxSet> syntax = getSyntax(scope);
		Auto<Parser> parser = CREATE(Parser, this, syntax, contents->v, contents->pos);
		nat r = parser->parse(L"FunctionBody");
		if (parser->hasError())
			throw parser->error();

		Auto<Object> c = parser->transform(vector<Object *>(1, this));
		Auto<FnBody> body = c.expect<FnBody>(engine(), L"While evaluating FunctionBody");

		return body.ret();
	}


	bs::FnBody::FnBody(Par<BSRawFn> owner) : ExprBlock(owner->scope), type(owner->result) {
		owner->addParams(this);
	}

}
