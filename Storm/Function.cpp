#include "stdafx.h"
#include "Function.h"
#include "Type.h"

namespace storm {

	Function::Function(Value result, const String &name, const vector<Value> &params)
		: NameOverload(name, params),
		  result(result) {}

	Function::~Function() {}

	void Function::output(wostream &to) const {
		to << result << " " << name << "(";
		join(to, params, L", ");
		to << ")";
	}


	NativeFunction::NativeFunction(Value result, const String &name, const vector<Value> &params, void *ptr)
		: Function(result, name, params), fnPtr(ptr) {}

	void *NativeFunction::pointer() const {
		return fnPtr;
	}

}
