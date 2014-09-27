#include "stdafx.h"

#include "Thread.h"

namespace util {

	Thread::Thread() : running(null), end(false) {
		TODO("Implement native thread!");
		TODO("Add semaphore!");
		assert(false);
	}

	Thread::~Thread() {
		stopWait();
	}

	void Thread::stop() {
		end = true;
	}

	void Thread::stopWait() {
		stop();
		while (running) { Sleep(0); }
	}

	bool Thread::sameAsCurrent() {
		//CWinThread *thread = AfxGetThread();
		//return thread == running;
		assert(false);
		return false;
	}

	bool Thread::start(const Function<void, Thread::Control &> &fn) {
		if (isRunning()) return false;
		end = false;

		this->fn = fn;
		// running = AfxBeginThread(threadProc, this);

		return true;
	}

	UINT Thread::threadProc(LPVOID param) {
		Thread *me = (Thread *)param;

		Thread::Control control(*me);
		me->fn(control);

		// done, clean up
		// me->running = null;
		return 0;
	}

}
