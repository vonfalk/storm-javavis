#include "stdafx.h"
#include "Thread.h"

namespace storm {

	Thread::Thread() : osThread(os::Thread::invalid), create(null) {}

	Thread::Thread(const os::Thread &thread) : osThread(thread), create(null) {}

	Thread::Thread(DeclThread::CreateFn fn) : osThread(os::Thread::invalid), create(fn) {}

	Thread::~Thread() {
		// We need to call the destructor of 'thread', so we need to declare a destructor to tell
		// Storm that the destructor needs to be called.
	}

	void Thread::deepCopy(CloneEnv *env) {
		// 'osThread' is threadsafe anyway.
	}

	const os::Thread &Thread::thread() {
		// TODO: Make thread-safe!

		if (osThread == os::Thread::invalid) {
			if (create) {
				// TODO? make sure the newly created thread has been properly registered with the gc!
				osThread = (*create)(engine());
			} else {
				osThread = os::Thread::spawn(util::Fn<void>(), runtime::threadGroup(engine()));
			}
		}
		return osThread;
	}

	STORM_DEFINE_THREAD(Compiler);

}
