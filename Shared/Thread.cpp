#include "stdafx.h"
#include "Thread.h"

namespace storm {

	Thread::Thread() : osThread(os::Thread::spawn(Fn<void, void>(), threadGroup(engine()))) {}

	Thread::Thread(Thread *from) : osThread(from->thread()) {}

	Thread::Thread(os::Thread t) : osThread(t) {}

	Thread::Thread(DeclThread::CreateFn fn) : osThread(os::Thread::invalid), create(fn) {}

	Thread::~Thread() {}

	void Thread::stopThread() {
		// TODO: This might need synchronization!
		osThread = os::Thread::current();
	}

	const os::Thread &Thread::thread() {
		if (osThread == os::Thread::invalid)
			osThread = (*create)(engine());
		return osThread;
	}

}
