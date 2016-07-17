#include "stdafx.h"
#include "Thread.h"
#include "Engine.h"
#include "Gc.h"

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

#ifdef STORM_COMPILER
	static const GcType firstDesc = {
		GcType::tFixed,
		null, // Type
		sizeof(Thread), // stride/size
		0, // # of offsets
	};

	void *Thread::operator new(size_t s, First d) {
		return d.e.gc.alloc(&firstDesc);
	}

	void Thread::operator delete(void *mem, First d) {}
#endif

	STORM_DEFINE_THREAD(Compiler);

}
