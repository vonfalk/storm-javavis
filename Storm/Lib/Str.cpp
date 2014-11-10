#include "stdafx.h"
#include "Str.h"
#include "Type.h"

namespace storm {

	Str::Str() : Object() {}

	Str::Str(const Str &o) : Object(), v(o.v) {}

	Str::Str(const String &o) : Object(), v(o) {}

	nat Str::count() const {
		return v.size();
	}

	Bool Str::equals(Object *o) {
		if (!Object::equals(o))
			return false;
		Str *other = (Str *)o;
		return v == other->v;
	}

	Str *Str::toS() {
		addRef();
		return this;
	}

}
