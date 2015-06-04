#include "stdafx.h"
#include "Shared/Storm.h"
#include "Engine.h"

namespace storm {

	Thread *DeclThread::thread(Engine &e) const {
		return e.thread((uintptr_t)&dummy);
	}

	void DeclThread::force(Engine &e, Thread *to) {
		e.thread((uintptr_t)&dummy, to);
	}

}

