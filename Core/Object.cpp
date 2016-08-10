#include "stdafx.h"
#include "Object.h"
#include "Str.h"
#include "StrBuf.h"

namespace storm {

	Object::Object() {}

	Object::Object(const Object &o) {}

	Object::~Object() {}

	bool Object::isA(const Type *o) const {
		TODO(L"FIXME");
		return false;
	}

	void Object::deepCopy(CloneEnv *env) {}

	Bool Object::equals(Object *o) const {
		if (o)
			return type() == o->type();
		else
			return false;
	}

	Nat Object::hash() const {
		TODO(L"Implement me!");
		return 0;
	}

	Str *Object::toS() const {
		StrBuf *b = new (this) StrBuf();
		toS(b);
		return b->toS();
	}

	void Object::toS(StrBuf *buf) const {
		*buf << L"<TODO: Type> @" << (void *)this;
	}

	wostream &operator <<(wostream &to, const Object *o) {
		return to << o->toS()->c_str();
	}

	wostream &operator <<(wostream &to, const Object &o) {
		return to << o.toS()->c_str();
	}

}
