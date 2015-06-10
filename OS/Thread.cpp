#include "stdafx.h"
#include "Thread.h"
#include "UThread.h"
#include "Shared.h"
#include <process.h>

namespace os {

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

		// ThreadWait behavior. May be null.
		ThreadWait *wait;

		// Init.
		ThreadStart(const Fn<void, void> &fn, ThreadWait *wait) : sema(0), startFn(fn), wait(wait) {}

		// Remove one reference from 'data', if present. The started thread
		// should set one reference in 'data', so that it may not exit
		// before the 'start' function has returned and reclaimed at least one
		// reference.
		~ThreadStart() {
			if (data)
				data->release();
		}
	};

	// This one is system-specific
	static void startThread(ThreadStart &start);

	/**
	 * Thread data.
	 */

	ThreadData::ThreadData() : references(0), uState(this) {}

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
		ThreadData *t = currentThreadData();
		if (t)
			return Thread(t);

		// The first thread, create its data...
		static ThreadData firstData;
		// Keep the first thread from firing 'signal' all the time.
		static Thread first(&firstData);
		return first;
	}

	Thread Thread::spawn(const Fn<void, void> &fn) {
		ThreadStart start(fn, null);
		startThread(start);
		start.sema.down();

		return Thread(start.data);
	}

	Thread Thread::spawn(ThreadWait *wait) {
		Fn<void, void> f;
		ThreadStart start(f, wait);
		startThread(start);
		start.sema.down();

		return Thread(start.data);
	}

	void ThreadData::threadMain(ThreadStart &start) {
		ThreadData d;

		threadCreated();

		// Read data from 'start'.
		Fn<void, void> fn = start.startFn;
		ThreadWait *wait = start.wait;
		d.wait = wait;
		d.addRef();

		// Remember our identity.
		currentThreadData(&d);

		// Report back.
		start.data = &d;
		start.sema.up();
		// From here on, do not touch 'start'.

		fn();

		// Specific wait behavior?
		if (wait) {
			do {
				// Run any spawned UThreads.
				while (UThread::leave())
					;

			} while (d.wait && wait->wait());

			// Clean up the 'wait' structure.
			d.wait = null; // No more notifications, but we can not delete it yet!
		}

		while (true) {
			// Either we have more references, or more UThreads to run.
			// Either way, it does not hurt to try to run the UThreads.
			while (UThread::leave())
				;

			// If the refcount is zero, we can safely say that no one else
			// can increase it after this point (assuming no UThreads).
			// At that point no one can add more UThreads either, so in
			// this case we can not have any race-conditions.

			if (atomicRead(d.references) == 0) {
				if (!UThread::any())
					break;
			}

			// Wait for the condition to fire. This is done whenever the
			// refcount reaches zero, or when a new UThread has been added.
			d.wakeCond.wait();
		}

		// Now, no one has any knowledge of our existence, we can safely delete the 'wait' now.
		delete wait;

		// Failsafe for the currThreadData.
		currentThreadData(null);

		threadTerminated();
	}

	void ThreadData::reportWake() {
		// We need to signal both in case we're in the process of exiting from the 'wait' behaviour.
		wakeCond.signal();
		if (wait)
			wait->signal();
	}

	void ThreadData::waitForWork() {
		if (wait) {
			if (!wait->wait())
				wait = null;
		} else {
			wakeCond.wait();
		}
	}

#ifdef WINDOWS
	// Windows-specific implementation.

	static void winThreadMain(void *param) {
		ThreadStart *s = (ThreadStart *)param;
		ThreadData::threadMain(*s);
	}

	static void startThread(ThreadStart &start) {
		_beginthread(&winThreadMain, 0, &start);
	}

#endif

}
