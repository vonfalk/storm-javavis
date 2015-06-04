#include "stdafx.h"
#include "SyntaxObject.h"
#include "Shared/Str.h"

namespace storm {

	SObject::SObject() : pos() {}

	SStr::SStr(Par<Str> s) : v(s) {}

	SStr::SStr(Par<Str> src, const SrcPos &pos) : v(src) {
		this->pos = pos;
	}

	SStr::SStr(Par<SStr> s) : v(s->v) {
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
