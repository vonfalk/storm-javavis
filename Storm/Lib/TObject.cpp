#include "stdafx.h"
#include "TObject.h"
#include "Thread.h"

namespace storm {

	TObject::TObject(Par<Thread> thread) : thread(thread) {}

	TObject::TObject(Par<TObject> c) : Object(c.borrow()), thread(c->thread) {}

	TObject::~TObject() {}

	Size TObject::baseSize() {
		Size s = Object::baseSize();
		s += Size::sPtr; // thread
		assert(s.current() == sizeof(Object), L"Forgot to update baseSize!");
		return s;
	}

	Offset TObject::threadOffset() {
		Offset r(Object::baseSize());
		assert(r.current() == OFFSET_OF(TObject, thread), L"Forgot to update threadOffset!");
		return r;
	}

}
