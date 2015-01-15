#include "stdafx.h"
#include "TypeVar.h"
#include "Type.h"

namespace storm {

	TypeVar::TypeVar(Type *owner, const Value &type, const String &name)
		: NameOverload(name, vector<Value>(1, owner)), varType(type) {}

	void TypeVar::output(wostream &to) const {
		to << varType << L" " << owner()->identifier() << L":" << name;
	}

	Named *TypeVar::find(const Name &name) {
		return null;
	}

	Type *TypeVar::owner() const {
		return params[0].type;
	}

	Offset TypeVar::offset() const {
		return owner()->offset(this);
	}
}
