#include "stdafx.h"
#include "Str.h"
#include "Type.h"

namespace storm {

	Str::Str(Type *type) : Object(type) {}

	Str::Str(Type *type, const Str &o) : Object(type), v(o.v) {}

	nat Str::count() const {
		return v.size();
	}


	// Create the string type.
	Type *strType() {
		return new Type(L"Str", typeClass);
	}

}
