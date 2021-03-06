#include "stdafx.h"
#include "Thread.h"

namespace storm {

	Thread::Thread() : osThread(os::Thread::invalid), create(null) {}

	Thread::Thread(const os::Thread &thread) : osThread(thread), create(null) {}

	Thread::Thread(DeclThread::CreateFn fn) : osThread(os::Thread::invalid), create(fn) {}

	Thread::Thread(const Thread &o) : osThread(const_cast<Thread &>(o).thread()), create(null) {}

	Thread::~Thread() {
		// We need to call the destructor of 'thread', so we need to declare a destructor to tell
		// Storm that the destructor needs to be called.
	}

	void Thread::deepCopy(CloneEnv *env) {
		// 'osThread' is threadsafe anyway.
	}

	const os::Thread &Thread::thread() {
		// Do not acquire the lock first, as creation will only happen once.
		if (osThread == os::Thread::invalid) {
			// Acquire lock and check again.
			util::Lock::L z(runtime::threadLock(engine()));
			if (osThread == os::Thread::invalid) {
				if (create) {
					// TODO: make sure the newly created thread has been properly registered with the gc?
					osThread = (*create)(engine());
				} else {
					osThread = os::Thread::spawn(util::Fn<void>(), runtime::threadGroup(engine()));
				}
			}
		}
		return osThread;
	}

	bool Thread::sameAs(const os::Thread &o) const {
		return o == osThread;
	}

	STORM_DEFINE_THREAD(Compiler);

}
