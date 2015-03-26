#include "stdafx.h"
#include "Thread.h"

namespace storm {

	DEFINE_STORM_THREAD(Compiler);

	Thread::Thread() : thread(code::Thread::spawn(Fn<void, void>())) {}

	Thread::Thread(Thread *from) : thread(from->thread) {}

	Thread::Thread(code::Thread t) : thread(t) {}

	Thread::~Thread() {}

}
