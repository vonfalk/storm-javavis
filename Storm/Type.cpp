#include "stdafx.h"
#include "Type.h"

namespace storm {

	Type::Type(const String &name, TypeFlags f) : name(name), flags(f) {}

	Type::~Type() {}

	void Type::output(wostream &to) const {
		to << L"Type " << name;
	}

}
