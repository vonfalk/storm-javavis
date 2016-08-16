#include "stdafx.h"
#include "Object.h"

namespace storm {

	Object::Object() {}

	Object::Object(const Object &o) {}

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
		return RootObject::toS();
	}

	void Object::toS(StrBuf *buf) const {
		RootObject::toS(buf);
	}
}
