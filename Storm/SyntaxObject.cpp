#include "stdafx.h"
#include "SyntaxObject.h"
#include "Lib/Str.h"

namespace storm {

	SObject::SObject() : pos() {}

	SStr::SStr(Auto<Str> s) : v(s) {}

	SStr::SStr(Auto<SStr> s) : v(s->v) {
		pos = s->pos;
	}

	SStr::SStr(const String &str) {
		v = CREATE(Str, engine(), str);
	}

	Bool SStr::equals(Object *o) {
		if (!Object::equals(o))
			return false;

		return v->equals(((SStr *)o)->v.ref());
	}

	void SStr::output(wostream &to) const {
		to << *v << L"@" << pos;
	}
}
