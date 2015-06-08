#include "stdafx.h"
#include "Thread.h"

namespace storm {

	DEFINE_STORM_THREAD(Compiler);

	Thread::Thread() : thread(os::Thread::spawn(Fn<void, void>())) {}

	Thread::Thread(Thread *from) : thread(from->thread) {}

	Thread::Thread(os::Thread t) : thread(t) {}

	Thread::~Thread() {}

}
