#include "stdafx.h"
#include "Thread.h"
#include "UThread.h"
#include "ThreadGroup.h"
#include "Shared.h"
#include <process.h>

#ifdef WINDOWS
// For COM - CoInitializeEx
#include <Objbase.h>
#endif

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
		const util::Fn<void, void> &startFn;

		// ThreadWait behavior. May be null.
		ThreadWait *wait;

		// Thread group.
		ThreadGroupData *group;

		// Init.
		ThreadStart(const util::Fn<void, void> &fn, ThreadWait *wait, ThreadGroupData *group) :
			sema(0), startFn(fn), wait(wait), group(group) {}

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
		if (data)
			data->addRef();
	}

	Thread::Thread(const Thread &o) : data(o.data) {
		if (data)
			data->addRef();
	}

	Thread::~Thread() {
		if (data)
			data->release();
	}

	Thread &Thread::operator =(const Thread &o) {
		if (data)
			data->release();
		data = o.data;
		if (data)
			data->addRef();
		return *this;
	}

	bool Thread::operator ==(const Thread &o) const {
		return data == o.data;
	}

	wostream &operator <<(wostream &to, const Thread &o) {
		return to << L"thread @" << o.data;
	}

	void Thread::attach(Handle h) const {
		data->attach(h);
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

	Thread Thread::invalid = Thread(null);

	const util::InlineSet<UThreadStack> &Thread::stacks() const {
		return data->uState.stacks;
	}

	Thread Thread::spawn(ThreadGroup &group) {
		return spawn(util::Fn<void>(), group);
	}

	Thread Thread::spawn(const util::Fn<void, void> &fn, ThreadGroup &group) {
		ThreadStart start(fn, null, group.data);
		startThread(start);
		start.sema.down();

		return Thread(start.data);
	}

	Thread Thread::spawn(ThreadWait *wait, ThreadGroup &group) {
		util::Fn<void, void> f;
		ThreadStart start(f, wait, group.data);
		startThread(start);
		start.sema.down();

		return Thread(start.data);
	}

	void ThreadData::threadMain(ThreadStart &start) {
		ThreadData d;

		Thread::initThread();
		threadCreated();

		// Read data from 'start'.
		ThreadGroupData *group = start.group;
		util::Fn<void, void> fn = start.startFn;
		ThreadWait *wait = start.wait;
		d.wait = wait;

		// One reference is consumed when 'threadWait' is terminated.
		d.addRef();
		// One is used to prevent signaling the semaphore before we have finished doing our main job.
		d.addRef();

		// Remember our identity.
		currentThreadData(&d);

		// Initialize our group.
		group->addRef();
		group->threadStarted();

		// Initialize any 'wait' struct before anyone is able to call 'signal' on it.
		if (wait)
			wait->init();

		// Report back.
		start.data = &d;
		start.sema.up();
		// From here on, do not touch 'start'.

		fn();

		// Specific wait behavior?
		if (wait) {
			do {
				// Run any spawned UThreads, interleave with anything we need to do.
				do {
					if (d.wait)
						wait->work();
				} while (UThread::leave());

			} while (d.wait && d.waitForWork());

			// Clean up the 'wait' structure.
			d.wait = null; // No more notifications, but we can not delete it yet!
		}

		// Go back to zero references, so that we may terminate!
		d.release();

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
			d.waitForWork();
		}

		// Now, no one has any knowledge of our existence, we can safely delete the 'wait' now.
		delete wait;

		// Report we terminated.
		group->threadTerminated();
		group->release();

		// Failsafe for the currThreadData.
		currentThreadData(null);

		if (d.ioComplete)
			d.ioComplete.close();
		threadTerminated();
		Thread::cleanThread();
	}

	void ThreadData::reportWake() {
		// We need to signal both in case we're in the process of exiting from the 'wait' behaviour.
		wakeCond.signal();
		if (wait)
			wait->signal();
	}

	bool ThreadData::waitForWork() {
		bool result = false;
		checkIo();

		nat sleepFor = 0;
		if (uState.nextWake(sleepFor)) {
			if (sleepFor > 0) {
				if (wait) {
					if (!wait->wait(ioComplete, sleepFor))
						wait = null;
					else
						result = true;
				} else {
					wakeCond.wait(ioComplete, sleepFor);
				}
			}
			uState.wakeThreads();
		} else {
			if (wait) {
				if (!wait->wait(ioComplete))
					wait = null;
				else
					result = true;
			} else {
				wakeCond.wait(ioComplete);
			}
		}

		checkIo();
		return result;
	}

	void ThreadData::checkIo() const {
		if (!ioComplete)
			return;

		ioComplete.notifyAll(this);
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

	void Thread::initThread() {
		CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_SPEED_OVER_MEMORY);
	}

	void Thread::cleanThread() {
		CoUninitialize();
	}

#else

	void Thread::initThread() {}

	void Thread::cleanThread() {}

#endif

	ThreadWait::~ThreadWait() {}

	void ThreadWait::init() {}

	void ThreadWait::work() {}

}
