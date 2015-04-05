#include "stdafx.h"
#include "BSFunction.h"
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
			Named *n = this->thread->find(scope);
			if (NamedThread *t = as<NamedThread>(n)) {
				thread = t;
			} else {
				throw SyntaxError(this->thread->pos, L"The identifier " + ::toS(thread) + L" is not a thread.");
			}
		}

		return CREATE(BSFunction, this, result, name->v->v, params, names, scope, contents, thread, name->pos, false);
	}


	bs::BSFunction::BSFunction(Value result, const String &name, const vector<Value> &params,
							const vector<String> &names, const Scope &scope, Par<SStr> contents,
							Par<NamedThread> thread, const SrcPos &pos, bool isMember)
		: Function(result, name, params), scope(scope), contents(contents),
		  paramNames(names), pos(pos), isMember(isMember), onThread(thread) {

		if (result.ref)
			throw SyntaxError(pos, L"Returning references is not a good idea at this point!");

		setCode(steal(CREATE(LazyCode, this, memberVoidFn(this, &BSFunction::generateCode))));
	}

	RunOn bs::BSFunction::runOn() const {
		if (onThread)
			return RunOn(onThread);

		return Function::runOn();
	}

	code::Listing bs::BSFunction::generateCode() {
		Auto<SyntaxSet> syntax = getSyntax(scope);

		Auto<Parser> parser = CREATE(Parser, this, syntax, contents->v, contents->pos);
		nat r = parser->parse(L"FunctionBody");
		if (parser->hasError())
			throw parser->error();

		Auto<Object> c = parser->transform(vector<Object *>(1, this));
		Auto<FnBody> body = c.expect<FnBody>(engine(), L"While evaluating FunctionBody");

		result.mustStore(body->result(), pos);

		// Generate code!
		using namespace code;
		Listing l;
		CodeData data;

		l << prolog();

		// Return value parameter (if needed).
		Variable returnValue;
		if (!result.returnOnStack()) {
			// Note: We do not need to free this one, since we will copy a value to it (and thereby initialize it)
			// the last thing we do in this function.
			returnValue = l.frame.createParameter(Size::sPtr, false);
		}

		GenState state = { l, data, runOn(), l.frame, l.frame.root() };

		// Parameters
		for (nat i = 0; i < params.size(); i++) {
			const Value &t = params[i];
			LocalVar *var = body->variable(paramNames[i]);
			assert(var);
			var->createParam(state);
		}

		if (result == Value()) {
			GenResult r;

			body->code(state, r);
			l << epilog();
			l << ret(Size());
		} else if (result.returnOnStack()) {
			GenResult r(result, l.frame.root());

			body->code(state, r);

			VarInfo rval = r.location(state);
			l << mov(asSize(ptrA, result.size()), rval.var);
			if (result.refcounted())
				l << code::addRef(ptrA);

			l << epilog();
			l << ret(result.size());
		} else {
			GenResult r(result, l.frame.root());

			body->code(state, r);

			VarInfo rval = r.location(state);
			l << lea(ptrA, ptrRel(rval.var));
			l << fnParam(returnValue);
			l << fnParam(ptrA);
			l << fnCall(result.copyCtor(), Size::sPtr);

			l << epilog();
			l << ret(Size::sPtr);
		}

		l << data;

		// PLN(identifier() << L": " << l);
		return l;
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
