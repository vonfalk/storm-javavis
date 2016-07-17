#include "stdafx.h"
#include "TObject.h"

namespace storm {

	TObject::TObject(Thread *t) : thread(t) {}

	TObject::TObject(TObject *o) : thread(o->thread) {}

	Bool TObject::equals(Object *o) const {
		return o == this;
	}

	Nat TObject::hash() const {
		return Nat(this);
	}

	Str *TObject::toS() const {
		return Object::toS();
	}

	void TObject::toS(StrBuf *buf) const {
		Object::toS(buf);
	}

}
