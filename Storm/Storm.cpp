#include "stdafx.h"
#include "Storm.h"
#include "Engine.h"

namespace storm {

	Thread *DeclThread::thread(Engine &e) const {
		return e.thread((uintptr_t)&createFn, createFn);
	}

	void DeclThread::force(Engine &e, Thread *to) {
		assert(createFn == null, L"It is probably not a good idea to force a thread with a special WAIT function.");
		e.thread((uintptr_t)&createFn, to);
	}


}
