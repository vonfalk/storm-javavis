#include "stdafx.h"
#include "Thread.h"
#include "Engine.h"
#include "Gc.h"

namespace storm {

	Thread::Thread() : osThread(os::Thread::spawn(Thread::registerFn(engine()), engine().threadGroup)) {}

	Thread::Thread(const os::Thread &thread) : osThread(thread) {}

	Thread::Thread(Thread *o) : osThread(o->thread()) {
		// We're using thread() above to ensure that the thread has been properly
		// initialized. Otherwise, we may get multiple initializations, which is bad.
		engine().gc.reattachThread(osThread);
	}

	Thread::~Thread() {
		if (osThread != os::Thread::invalid)
			engine().gc.detachThread(osThread);
	}

	void Thread::deepCopy(CloneEnv *env) {
		// 'osThread' is threadsafe anyway.
	}

	const os::Thread &Thread::thread() {
		if (osThread == os::Thread::invalid) {
			// TODO? make sure the newly created thread has been properly registered with the gc!
			osThread = (*create)(engine());
		}
		return osThread;
	}

	Fn<void, void> Thread::registerFn(Engine &e) {
		return Fn<void, void>(&e.gc, &Gc::attachThread);
	}

#ifdef STORM_COMPILER
	static void destroyThread(Thread *t) {
		t->~Thread();
	}

	static const GcType firstDesc = {
		GcType::tFixed,
		null, // Type
		address(&destroyThread), // Finalizer
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
