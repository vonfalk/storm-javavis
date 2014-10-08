#include "stdafx.h"
#include "Type.h"

namespace storm {

	Type::Type(const String &name, TypeFlags f) : name(name), flags(f) {}

	Type::~Type() {}

	nat Type::size() const {
		TODO(L"Implement me!");
		return 0;
	}

	void Type::output(wostream &to) const {
		to << name;
	}

}
