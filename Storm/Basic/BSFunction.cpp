#include "stdafx.h"
#include "BSFunction.h"
#include "Code/Instruction.h"
#include "Parser.h"
#include "PkgReader.h"
#include "Exception.h"

namespace storm {

	bs::FunctionDecl::FunctionDecl(SrcPos pos,
								Par<SStr> name,
								Par<TypeName> result,
								Par<Params> params,
								Par<SStr> contents)
		: pos(pos), name(name), result(result), params(params), contents(contents) {}

	Function *bs::FunctionDecl::asFunction(const Scope &scope) {
		Value result = this->result->value(scope);
		vector<Value> params = this->params->cTypes(scope);
		vector<String> names = this->params->cNames();

		return CREATE(BSFunction, this, result, name->v->v, params, names, scope, contents, name->pos);
	}


	bs::BSFunction::BSFunction(Value result, const String &name, const vector<Value> &params,
							const vector<String> &names, const Scope &scope,
							Par<SStr> contents, const SrcPos &pos)
		: Function(result, name, params), scope(scope), contents(contents), paramNames(names) {

		if (result.ref)
			throw SyntaxError(pos, L"Returning references is not a good idea at this point!");

		setCode(steal(CREATE(LazyCode, this, memberVoidFn(this, &BSFunction::generateCode))));
	}

	code::Listing bs::BSFunction::generateCode() {
		SyntaxSet syntax;
		addSyntax(scope, syntax);

		Parser parser(syntax, contents->v->v, contents->pos);
		nat r = parser.parse(L"FunctionBody");
		if (parser.parse(L"FunctionBody") < contents->v->v.size())
			throw parser.error();

		Auto<Object> c = parser.transform(engine(), vector<Object *>(1, this));
		Auto<FnBody> body = c.expect<FnBody>(engine(), L"While evaluating FunctionBody");

		result.mustStore(body->result(), pos);

		// Generate code!
		using namespace code;
		Listing l;

		l << prolog();

		// Return value parameter (if needed).
		Variable returnValue;
		if (!result.returnOnStack()) {
			returnValue = l.frame.createParameter(Size::sPtr, false, result.destructor(), freeOnException);
		}

		// Parameters
		for (nat i = 0; i < params.size(); i++) {
			const Value &t = params[i];
			LocalVar *var = body->variable(paramNames[i]);
			assert(var);
			if (t.isValue()) {
				var->var = l.frame.createParameter(t.size(), false, t.destructor(), freeOnBoth | freePtr);
			} else {
				var->var = l.frame.createParameter(t.size(), false, t.destructor());
			}

			if (t.refcounted())
				l << code::addRef(var->var);
		}

		GenState state = { l, l.frame, l.frame.root() };

		if (result == Value()) {
			GenResult r;

			body->code(state, r);
			l << epilog();
			l << ret(Size());
		} else if (result.returnOnStack()) {
			GenResult r(result, l.frame.root());

			body->code(state, r);

			Variable rval = r.location(state);
			l << mov(asSize(ptrA, result.size()), rval);
			if (result.refcounted())
				l << code::addRef(ptrA);

			l << epilog();
			l << ret(result.size());
		} else {
			GenResult r(result, l.frame.root());

			body->code(state, r);

			Variable rval = r.location(state);
			l << lea(ptrA, ptrRel(rval));
			l << fnParam(returnValue);
			l << fnParam(ptrA);
			l << fnCall(result.copyCtor(), Size::sPtr);

			l << epilog();
			l << ret(Size::sPtr);
		}

		// PLN(identifier() << L": " << l);
		return l;
	}

	void bs::BSFunction::addParams(Par<Block> to) {
		for (nat i = 0; i < params.size(); i++) {
			Auto<LocalVar> var = CREATE(LocalVar, this, paramNames[i], params[i], pos, true);
			to->add(var);
		}
	}

	bs::FnBody::FnBody(Par<BSFunction> owner) : ExprBlock(owner->scope) {
		owner->addParams(this);
	}

}
