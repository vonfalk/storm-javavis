#include "stdafx.h"
#include "Function.h"
#include "Type.h"

namespace storm {

	Function::Function(Type *result, const String &name, const vector<Type*> &params)
		: result(result),
		  name(name),
		  params(params) {}

	Function::~Function() {}

	void Function::output(wostream &to) const {
		to << ::toS(*result) << " " << name << "(";
		join(to, params, L", ");
		to << ")";
	}

}
