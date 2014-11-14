#include "stdafx.h"
#include "Str.h"
#include "Type.h"

namespace storm {

	Str::Str() : Object() {}

	Str::Str(Auto<Str> o) : Object(), v(o->v) {}

	Str::Str(const String &o) : Object(), v(o) {}

	nat Str::count() const {
		return v.size();
	}

	Bool Str::equals(Auto<Object> o) {
		if (!Object::equals(o))
			return false;
		Str *other = (Str *)o.borrow();
		return v == other->v;
	}

	void Str::output(wostream &to) const {
		to << v;
	}

	Str *Str::toS() {
		addRef();
		return this;
	}

}
