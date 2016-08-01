#include "stdafx.h"
#include "Thread.h"

namespace storm {

	Thread::Thread() : osThread(os::Thread::spawn(Thread::registerFn(engine()), runtime::threadGroup(engine()))) {}

	Thread::Thread(const os::Thread &thread) : osThread(thread) {}

	Thread::Thread(Thread *o) : osThread(o->thread()) {
		// We're using thread() above to ensure that the thread has been properly
		// initialized. Otherwise, we may get multiple initializations, which is bad.
		runtime::reattachThread(engine(), osThread);
	}

	Thread::~Thread() {
		if (osThread != os::Thread::invalid) {
			TODO(L"Make sure we do not detach a thread until it has stopped running (which may happen after the destructor is called)");
			runtime::detachThread(engine(), osThread);
		}
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
