#include "stdafx.h"
#include "TObject.h"

namespace storm {

	TObject::TObject(Par<Thread> thread) : thread(thread) {}

	TObject::TObject(Par<TObject> c) : Object(c.borrow()), thread(c->thread) {}

	TObject::~TObject() {}

	Bool TObject::equals(Par<Object> o) {
		return o.borrow() == this;
	}

	Nat TObject::hash() {
		return Nat(this);
	}

}
