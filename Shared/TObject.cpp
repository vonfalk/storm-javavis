#include "stdafx.h"
#include "TObject.h"

namespace storm {

	TObject::TObject(Par<Thread> thread) : thread(thread) {}

	TObject::TObject(Par<TObject> c) : Object(c.borrow()), thread(c->thread) {}

	TObject::~TObject() {}

	static void deleteTObject(TObject *o) {
		delete o;
	}

	void TObject::deleteMe() {
		// TODO: It would be good to know when the threads in the system has been killed, so that we
		// can just delete the object without trying (in vain) to delete the object on another thread.

		const os::Thread &on = thread->thread();
		if (on == os::Thread::current()) {
			// Correct thread.
			delete this;
		} else {
			// We need to post it.
			os::FnParams::Param buf;
			os::FnParams p(&buf);
			p.add(this);
			os::UThread::spawn(&deleteTObject, false, p, &on);
		}
	}

	Bool TObject::equals(Par<Object> o) {
		return o.borrow() == this;
	}

	Nat TObject::hash() {
		return Nat(this);
	}

}
