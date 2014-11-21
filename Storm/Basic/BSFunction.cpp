#include "stdafx.h"
#include "BSFunction.h"

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
		: LazyFunction(result, name, params), contents(contents) {}

}
