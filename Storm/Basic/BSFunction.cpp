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
			params.push_back(this->params->params[i]->value(scope));

		return CREATE(BSFunction, this, result, name->v->v, params, contents->v);
	}


	bs::BSFunction::BSFunction(Value result, const String &name, const vector<Value> &params, Auto<Str> contents)
		: Function(result, name, params), contents(contents) {

		setCode(CREATE(LazyCode, this, memberVoidFn(this, &BSFunction::generateCode)));
	}

	code::Listing bs::BSFunction::generateCode() {
		Path file(L"foo.bs");
		SyntaxSet syntax;
		syntax.add(*engine().package(syntaxPkg(file)));
		TODO(L"Respect use statements, proper file.");

		Parser parser(syntax, contents->v);
		nat r = parser.parse(L"FunctionBody");
		if (parser.parse(L"FunctionBody") < contents->v.size())
			throw parser.error(file);

		Auto<Object> c = parser.transform(engine(), file);
		Auto<FnBody> body = c.as<FnBody>();
		if (!body)
			throw InternalError(L"Invalid syntax");

		using namespace code;
		Listing l;
		l << mov(eax, natConst(2));
		l << ret(4);
		return l;
	}

	bs::FnBody::FnBody() {}

	void bs::FnBody::expr(Auto<Expr> expr) {}

}
