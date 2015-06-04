#include "stdafx.h"
#include "Str.h"

namespace storm {

	Str::Str() : Object() {}

	Str::Str(Par<Str> o) : Object(), v(o->v) {}

	Str::Str(const String &o) : Object(), v(o) {}

	Str::Str(const wchar *s) : Object(), v(s) {}

	nat Str::count() const {
		return v.size();
	}

	Str *Str::operator +(Par<Str> o) {
		return CREATE(Str, this, v + o->v);
	}

	Str *Str::operator *(Nat times) {
		String result(v.size() * times, ' ');
		for (nat i = 0; i < times; i++) {
			for (nat j = 0; j < v.size(); j++) {
				result[i*v.size() + j] = v[j];
			}
		}
		return CREATE(Str, this, result);
	}

	Bool Str::equals(Par<Object> o) {
		if (!Object::equals(o))
			return false;
		Str *other = (Str *)o.borrow();
		return v == other->v;
	}

	Int Str::toInt() const {
		return v.toInt();
	}

	Nat Str::toNat() const {
		return v.toNat();
	}

	void Str::output(wostream &to) const {
		to << v;
	}

	Str *Str::toS() {
		addRef();
		return this;
	}

	Str *Str::createStr(Type *type, const wchar *str) {
		return new (type) Str(str);
	}

}
