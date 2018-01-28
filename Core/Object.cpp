#include "stdafx.h"
#include "Object.h"

namespace storm {

	Object::Object() {}

	void Object::deepCopy(CloneEnv *env) {}

	Str *Object::toS() const {
		return RootObject::toS();
	}

	void Object::toS(StrBuf *buf) const {
		RootObject::toS(buf);
	}
}
