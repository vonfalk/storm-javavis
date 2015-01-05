#include "stdafx.h"
#include "BSFunction.h"
#include "Code/Instruction.h"
#include "Parser.h"
#include "PkgReader.h"
#include "Exception.h"

namespace storm {

	bs::FunctionDecl::FunctionDecl(SrcPos pos,
								Auto<SStr> name,
								Auto<TypeName> result,
								Auto<Params> params,
								Auto<SStr> contents)
		: pos(pos), name(name), result(result), params(params), contents(contents) {}

	Function *bs::FunctionDecl::asFunction(const Scope &scope) {
		Value result = this->result->value(scope);
		vector<Value> params(this->params->params.size());
		for (nat i = 0; i < this->params->params.size(); i++)
			params[i] = this->params->params[i]->value(scope);

		vector<String> names(this->params->names.size());
		for (nat i = 0; i < this->params->names.size(); i++)
			names[i] = this->params->names[i]->v->v;

		return CREATE(BSFunction, this, result, name->v->v, params, names, scope, contents, name->pos);
	}


	bs::BSFunction::BSFunction(Value result, const String &name, const vector<Value> &params,
							const vector<String> &names, const Scope &scope,
							Auto<SStr> contents, const SrcPos &pos)
		: Function(result, name, params), scope(scope), contents(contents), paramNames(names) {

		if (result.ref)
			throw SyntaxError(pos, L"Returning references is not a good idea at this point!");

		setCode(CREATE(LazyCode, this, memberVoidFn(this, &BSFunction::generateCode)));
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

		// Parameters
		for (nat i = 0; i < params.size(); i++) {
			const Value &t = params[i];
			LocalVar *var = body->variable(paramNames[i]);
			assert(var);
			var->var = l.frame.createParameter(t.size(), false, t.destructor());
		}

		GenState state = { l, l.frame, l.frame.root() };

		if (result == Value()) {
			GenResult r;

			l << prolog();
			body->code(state, r);
			l << epilog();
			l << ret(0);
		} else {
			GenResult r(result, l.frame.root());

			l << prolog();
			body->code(state, r);

			Variable result = r.location(state);
			l << mov(asSize(ptrA, result.size()), result);
			l << epilog();
			l << ret(result.size());
		}

		PLN(identifier() << L": " << l);
		return l;
	}

	void bs::BSFunction::addParams(Auto<Block> to) {
		for (nat i = 0; i < params.size(); i++) {
			Auto<LocalVar> var = CREATE(LocalVar, this, paramNames[i], params[i], pos, true);
			to->add(var);
		}
	}

	bs::FnBody::FnBody(Auto<BSFunction> owner) : ExprBlock(owner->scope) {
		owner->addParams(capture(this));
	}

}
