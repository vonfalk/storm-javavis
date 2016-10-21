#include "stdafx.h"
#include "Thread.h"

namespace storm {

	Thread::Thread() : osThread(os::Thread::invalid), create(null) {}

	Thread::Thread(const os::Thread &thread) : osThread(thread), create(null) {}

	Thread::Thread(DeclThread::CreateFn fn) : osThread(os::Thread::invalid), create(fn) {}

	Thread::Thread(Thread *o) : osThread(o->thread()), create(null) {
		// We're using thread() above to ensure that the thread has been properly
		// initialized. Otherwise, we may get multiple initializations, which is bad.
		runtime::reattachThread(engine(), osThread);
	}

	Thread::~Thread() {
		if (osThread != os::Thread::invalid) {
			TODO(L"Make sure we do not detach a thread until it has stopped running (which may happen after the destructor is called)");
			// Sometimes the GcType is not properly set for thread objects, try not to crash in that case.
			if (type()) {
				runtime::detachThread(engine(), osThread);
			}
		}
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
				osThread = os::Thread::spawn(Thread::registerFn(engine()), runtime::threadGroup(engine()));
			}
		}
		return osThread;
	}

	Fn<void, void> Thread::registerFn(Engine &e) {
		// This is an ugly hack to get around the limitation that Fn<> only supports 'this' as parameters.
		struct Wrap {
			void attach() {
				runtime::attachThread((Engine &)*this);
			}
		};

		return Fn<void, void>((Wrap *)&e, &Wrap::attach);
	}

	STORM_DEFINE_THREAD(Compiler);

}
