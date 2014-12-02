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

	Function *bs::FunctionDecl::asFunction(Auto<BSScope> scope) {
		Value result = this->result->value(scope);
		vector<Value> params(this->params->params.size());
		for (nat i = 0; i < this->params->params.size(); i++)
			params.push_back(this->params->params[i]->value(scope));

		return CREATE(BSFunction, this, result, name->v->v, params, scope, contents->v);
	}


	bs::BSFunction::BSFunction(Value result, const String &name, const vector<Value> &params,
							Auto<BSScope> scope, Auto<Str> contents)
		: Function(result, name, params), scope(scope), contents(contents) {

		setCode(CREATE(LazyCode, this, memberVoidFn(this, &BSFunction::generateCode)));
	}

	code::Listing bs::BSFunction::generateCode() {
		const Path &file = scope->file;
		SyntaxSet syntax;
		scope->addSyntax(syntax);

		Parser parser(syntax, contents->v);
		nat r = parser.parse(L"FunctionBody");
		if (parser.parse(L"FunctionBody") < contents->v.size())
			throw parser.error(file);

		Auto<Object> c = parser.transform(engine(), file, vector<Object *>(1, scope.borrow()));
		Auto<FnBody> body = c.as<FnBody>();
		if (!body)
			throw InternalError(L"Invalid syntax");

		Value resultType = body->result();
		result.mustStore(result, SrcPos(file, 0));

		// Generate code!
		using namespace code;
		Listing l;

		GenState state = { l, l.frame, l.frame.root() };

		if (resultType == Value()) {
			l << prolog();
			body->code(state, Variable::invalid);
			l << epilog();
			l << ret(0);
		} else {
			code::Block root = l.frame.root();
			Variable resultVar = l.frame.createVariable(root, resultType.size(), resultType.destructor());

			l << prolog();
			body->code(state, resultVar);
			l << mov(asSize(ptrA, resultType.size()), resultVar);
			l << epilog();
			l << ret(resultType.size());
		}

		PVAR(l);
		return l;
	}

	bs::FnBody::FnBody(Auto<BSScope> scope) : Block(scope) {}

}
