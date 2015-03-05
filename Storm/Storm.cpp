#include "stdafx.h"
#include "Storm.h"
#include "Engine.h"

namespace storm {

	Thread *DeclThread::thread(Engine &e) const {
		return e.thread((uintptr_t)&dummy);
	}

}

