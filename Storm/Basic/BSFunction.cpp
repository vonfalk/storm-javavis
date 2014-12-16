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
			params[i] = this->params->params[i]->value(scope);

		vector<String> names(this->params->names.size());
		for (nat i = 0; i < this->params->names.size(); i++)
			names[i] = this->params->names[i]->v->v;

		return CREATE(BSFunction, this, result, name->v->v, params, names, scope, contents->v, name->pos);
	}


	bs::BSFunction::BSFunction(Value result, const String &name, const vector<Value> &params,
							const vector<String> &names, Auto<BSScope> scope,
							Auto<Str> contents,	const SrcPos &pos)
		: Function(result, name, params), scope(scope), contents(contents), paramNames(names) {

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

		Auto<Object> c = parser.transform(engine(), file, vector<Object *>(1, this));
		Auto<FnBody> body = c.as<FnBody>();
		if (!body)
			throw InternalError(L"Invalid syntax");

		Value resultType = body->result();
		result.mustStore(resultType, SrcPos(file, 0));

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

		if (resultType == Value()) {
			GenResult r;

			l << prolog();
			body->code(state, r);
			l << epilog();
			l << ret(0);
		} else {
			GenResult r(l.frame.root());

			l << prolog();
			body->code(state, r);

			Variable result = r.location(state, resultType);
			l << mov(asSize(ptrA, resultType.size()), result);
			l << epilog();
			l << ret(resultType.size());
		}

		PVAR(l);
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
