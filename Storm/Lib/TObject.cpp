#include "stdafx.h"
#include "TObject.h"
#include "Thread.h"

namespace storm {

	TObject::TObject(Par<Thread> thread) : thread(thread) {}

	TObject::TObject(Par<TObject> c) : Object(c.borrow()), thread(c->thread) {}

	Size TObject::baseSize() {
		Size s = Object::baseSize();
		s += Size::sPtr; // thread
		assert(s.current() == sizeof(Object), L"Forgot to update baseSize!");
		return s;
	}

}
