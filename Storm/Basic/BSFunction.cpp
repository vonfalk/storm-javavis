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
		vector<Value> params = this->params->cTypes(scope);
		vector<String> names = this->params->cNames();
		NamedThread *thread = null;

		if (this->thread) {
			Auto<Named> n = this->thread->find(scope);
			if (NamedThread *t = as<NamedThread>(n.borrow())) {
				thread = t;
			} else {
				throw SyntaxError(this->thread->pos, L"The identifier " + ::toS(thread) + L" is not a thread.");
			}
		}

		return CREATE(BSFunction, this, result, name->v->v, params, names, scope, contents, thread, name->pos, false);
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


	bs::BSFunction::BSFunction(Value result, const String &name, const vector<Value> &params,
							const vector<String> &names, const Scope &scope, Par<SStr> contents,
							Par<NamedThread> thread, const SrcPos &pos, bool isMember)
		: Function(result, name, params), scope(scope), contents(contents),
		  paramNames(names), pos(pos), isMember(isMember) {

		if (thread)
			runOn(thread);

		if (result.ref)
			throw SyntaxError(pos, L"Returning references is not a good idea at this point!");

		Auto<FnPtr<CodeGen *>> fn = memberWeakPtr(engine(), this, &BSFunction::generateCode);
		setCode(steal(CREATE(LazyCode, this, fn)));
	}

	void bs::BSFunction::update(const vector<String> &names, Par<SStr> contents, const SrcPos &pos) {
		paramNames = names;
		this->contents = contents;
		this->pos = pos;

		// Could be done better...
		Auto<FnPtr<CodeGen *>> fn = memberWeakPtr(engine(), this, &BSFunction::generateCode);
		setCode(steal(CREATE(LazyCode, this, fn)));
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

		Auto<FnPtr<CodeGen *>> fn = memberWeakPtr(engine(), this, &BSFunction::generateCode);
		setCode(steal(CREATE(LazyCode, this, fn)));
	}


	static code::Variable createResultVar(code::Listing &l) {
		// Note: We do not need to free this one, since we will copy a value to it (and thereby initialize it)
		// the last thing we do in this function.
		return l.frame.createParameter(Size::sPtr, false);
	}

	CodeGen *bs::BSFunction::generateCode() {
		Auto<SyntaxSet> syntax = getSyntax(scope);

		Auto<Parser> parser = CREATE(Parser, this, syntax, contents->v, contents->pos);
		nat r = parser->parse(L"FunctionBody");
		if (parser->hasError())
			throw parser->error();

		Auto<Object> c = parser->transform(vector<Object *>(1, this));
		Auto<FnBody> body = c.expect<FnBody>(engine(), L"While evaluating FunctionBody");

		// Expression possibly wrapped around the body (casting the value if needed).
		Auto<Expr> bodyExpr = expectCastTo(body, result);

		// Generate code!
		using namespace code;
		Auto<CodeGen> state = CREATE(CodeGen, this, runOn());

		Listing &l = state->to;

		l << prolog();

		// Return value parameter (if needed).
		Variable returnValue;
		nat start = 0;
		if (!result.returnInReg()) {
			if (isMember) {
				// In member functions, the this ptr comes first!
				LocalVar *var = body->variable(paramNames[0]);
				var->createParam(state);
				start = 1;
			}

			returnValue = createResultVar(l);
		}

		// Parameters
		for (nat i = start; i < params.size(); i++) {
			LocalVar *var = body->variable(paramNames[i]);
			assert(var);
			var->createParam(state);
		}

		assert(result.returnInReg() || returnValue != Variable::invalid);

		if (result == Value()) {
			Auto<CodeResult> r = CREATE(CodeResult, this);
			bodyExpr->code(state, r);
			l << epilog();
			l << ret(retVoid());
		} else if (result.returnInReg()) {
			Auto<CodeResult> r = CREATE(CodeResult, this, result, l.frame.root());

			bodyExpr->code(state, r);

			VarInfo rval = r->location(state);
			l << mov(asSize(ptrA, result.size()), rval.var());
			if (result.refcounted())
				l << code::addRef(ptrA);

			l << epilog();

			l << ret(result.retVal());
		} else {
			Auto<CodeResult> r = CREATE(CodeResult, this, result, l.frame.root());
			bodyExpr->code(state, r);

			VarInfo rval = r->location(state);
			l << lea(ptrA, ptrRel(rval.var()));
			l << fnParam(returnValue);
			l << fnParam(ptrA);
			l << fnCall(result.copyCtor(), retPtr());
			// We need to provide the address of the return value as our result. The copy ctor
			// does not neccessarily return an address to the created value.
			l << mov(ptrA, returnValue);

			l << epilog();
			l << ret(retPtr());
		}

		// if (!identifier().startsWith(L"lang.bs")) {
		// 	PLN(bodyExpr);
		// 	PLN(identifier() << L": " << l);
		// }
		return state.ret();
	}

	void bs::BSFunction::addParams(Par<Block> to) {
		for (nat i = 0; i < params.size(); i++) {
			Auto<LocalVar> var = CREATE(LocalVar, this, paramNames[i], params[i], pos, true);
			if (i == 0 && isMember)
				var->constant = true;
			to->add(var);
		}
	}

	bs::FnBody::FnBody(Par<BSFunction> owner) : ExprBlock(owner->scope) {
		owner->addParams(this);
	}

}
