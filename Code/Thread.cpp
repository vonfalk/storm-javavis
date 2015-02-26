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

	ThreadData::ThreadData() : references(0), doneSema(null) {}

	ThreadData::~ThreadData() {
		if (doneSema)
			doneSema->up();
	}


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
		static Thread first(new ThreadData());
		return first;
	}

	Thread Thread::start(const Fn<void, void> &fn) {
		ThreadStart start(fn);
		startThread(start);
		start.sema.down();

		return Thread(start.data);
	}

	void Thread::threadMain(ThreadStart &start) {
		Semaphore done(0);
		ThreadData *d = new ThreadData();
		d->doneSema = &done;
		// we need to keep this one alive until we terminate.
		d->addRef();

		currThreadData = d;
		start.data = d;
		Fn<void, void> fn = start.startFn;

		start.sema.up();
		// From here on, do not touch 'start'.

		fn();

		// Let any UThreads terminate...
		while (UThread::any())
			UThread::leave();

		// Release our reference, so that it may report no more usage.
		d->release();

		// Keep the thread alive until the ThreadData is destroyed.
		done.down();

		TODO(L"Someone may have added more UThreads at this point. They should be executed before done.down returns.");
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
