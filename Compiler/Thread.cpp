#include "stdafx.h"
#include "Thread.h"
#include "Engine.h"

namespace storm {

	Thread::Thread() : osThread(os::Thread::spawn(Fn<void, void>(), engine().threadGroup)) {}

	Thread::Thread(const os::Thread &thread) : osThread(thread) {}

	Thread::Thread(Thread *o) : osThread(o->thread()) {
		// We're using thread() above to ensure that the thread has been properly
		// initialized. Otherwise, we may get multiple initializations, which is bad.
	}

	Thread::~Thread() {
		// We only need the destructor declared to ensure it is executed!
	}

	void Thread::deepCopy(CloneEnv *env) {
		// 'osThread' is threadsafe anyway.
	}

	const os::Thread &Thread::thread() {
		if (osThread == os::Thread::invalid)
			osThread = (*create)(engine());
		return osThread;
	}


	STORM_DEFINE_THREAD(Compiler);

}
