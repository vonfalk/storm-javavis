#include "stdafx.h"
#include "Thread.h"

namespace storm {

	DEFINE_STORM_THREAD(Compiler);

	Thread::Thread() : thread(code::Thread::spawn(Fn<void, void>())) {
		PLN("Thread launched!");
	}

	Thread::~Thread() {
		PLN("Thread exited.");
	}

}
