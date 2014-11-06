#pragma once

#include "Thread.h"
#include "Function.h"
#include "Semaphore.h"
#include "Lock.h"

#include <list>

namespace util {

	// A thread used to manage computationally intensive tasks, so that
	// responsiveness will not be suffering.
	// The thread internally keeps a queue of the functions to call
	// from the started thread. The thread is started at the first request
	// and kept alive as long as this object.
	class WorkerThread : NoCopy {
	public:
		WorkerThread();

		// Destroying this object will block until the currently
		// running callback has returned. This can be checked by
		// calling "isRunning".
		~WorkerThread();

		// Check if there is anything running at the moment in this thread.
		bool isRunning();

		// This is a small control struct which encapsulates the
		// passing of thread notifications.
		class Control : NoCopy {
			friend class WorkerThread;
		public:
			// Returns true as long as someone is not trying to shut the thread down.
			inline bool run() { return control.run(); }

		private:
			inline Control(Thread::Control &c) : control(c) {}

			Thread::Control &control;
		};

		// Add a delayed call to the thread
		void add(Fn<void, Control &> &toCall);
	private:
		// The thread itself.
		Thread worker;

		// Lock for the list.
		Lock workLock;

		// List of all the work.
		std::list<Fn<void, Control &> > workList;

		// Semaphore for keeping the thread waiting.
		Semaphore waitSema;

		// The thread function.
		void threadMain(Thread::Control &control);
	};

}
