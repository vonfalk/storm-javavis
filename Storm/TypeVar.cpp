#include "stdafx.h"
#include "TypeVar.h"

namespace storm {

	TypeVar::TypeVar(Type *owner, const Value &type, const String &name)
		: NameOverload(name, vector<Value>(1, owner)), varType(type) {}

	void TypeVar::output(wostream &to) const {
		to << varType << L" " << name;
	}

	Named *TypeVar::find(const Name &name) {
		return null;
	}
}
