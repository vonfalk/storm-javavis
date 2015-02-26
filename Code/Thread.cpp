#include "stdafx.h"
#include "Thread.h"
#include "UThread.h"
#include <process.h>

namespace code {

	// Data to a started thread.
	class ThreadStart {
	public:
		// Semaphore to indicate successful start.
		Semaphore sema;

		// Pointer to the ThreadData object for the current thread.
		// Valid when 'sema' has been decreased.
		ThreadData *data;

		// Function to execute.
		const Fn<void, void> &startFn;

		// Init.
		ThreadStart(const Fn<void, void> &fn) : sema(0), startFn(fn) {}
	};

	// This one is system-specific
	static void startThread(ThreadStart &start);

	// This is 'null' for the first thread that is not spawned through 'startThread'.
	static THREAD ThreadData *currThreadData = 0;

	/**
	 * Thread data.
	 */

	ThreadData::ThreadData() : references(0) {}

	ThreadData::~ThreadData() {}

	void ThreadData::reportZero() {
		wakeCond.signal();
	}

	/**
	 * Thread
	 */

	Thread::Thread(ThreadData *data) : data(data) {
		data->addRef();
	}

	Thread::Thread(const Thread &o) : data(o.data) {
		data->addRef();
	}

	Thread::~Thread() {
		data->release();
	}

	Thread &Thread::operator =(const Thread &o) {
		data->release();
		data = o.data;
		data->addRef();
		return *this;
	}

	bool Thread::operator ==(const Thread &o) const {
		return data == o.data;
	}

	void Thread::output(wostream &to) const {
		to << L"thread @" << data;
	}


	Thread Thread::current() {
		if (currThreadData)
			return Thread(currThreadData);

		// The first thread, create its data...
		static ThreadData firstData;
		// Keep the first thread from firing 'signal' all the time.
		static Thread first(&firstData);
		return first;
	}

	Thread Thread::start(const Fn<void, void> &fn) {
		ThreadStart start(fn);
		startThread(start);
		start.sema.down();

		return Thread(start.data);
	}

	void Thread::threadMain(ThreadStart &start) {
		ThreadData d;

		currThreadData = &d;
		start.data = &d;
		Fn<void, void> fn = start.startFn;

		start.sema.up();
		// From here on, do not touch 'start'.

		fn();

		// Let any UThreads terminate...
		while (UThread::any())
			UThread::leave();

		while (true) {
			// Wait for the condition to fire. This is done whenever the
			// refcount reaches zero, or when a new UThread has been added.
			d.wakeCond.wait();

			// If the refcount is zero, we can safely say that no one else
			// can increase it after this point (assuming no UThreads).
			// At that point no one can add more UThreads either, so in
			// this case we can not have any race-conditions.

			// Hacky atomic read...
			if (atomicCAS(d.references, 0, 0) == 0)
				if (!UThread::any())
					break;

			// Either we have more references, or more UThreads to run.
			// Either way, it does not hurt to try to run the UThreads.
			while (UThread::any())
				UThread::leave();
		}

		// Failsafe for the currThreadData.
		currThreadData = null;
	}

#ifdef WINDOWS
	// Windows-specific implementation.

	static void winThreadMain(void *param) {
		ThreadStart *s = (ThreadStart *)param;
		Thread::threadMain(*s);
	}

	static void startThread(ThreadStart &start) {
		_beginthread(&winThreadMain, 0, &start);
	}

#endif

}
