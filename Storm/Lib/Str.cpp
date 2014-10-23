#include "stdafx.h"
#include "Str.h"
#include "Type.h"

namespace storm {

	Str::Str(Type *type) : Object(type) {}

	Str::Str(Type *type, const Str &o) : Object(type), v(o.v) {}

	Str::Str(Type *type, const String &o) : Object(type), v(o) {}

	nat Str::count() const {
		return v.size();
	}

}
