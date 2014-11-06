#include "StdAfx.h"
#include "WorkerThread.h"

namespace util {

	WorkerThread::WorkerThread() : waitSema(0, 100) {}

	WorkerThread::~WorkerThread() {
		worker.stop();
		// Release the worker if it is stuck in the semaphore.
		waitSema.up();

		worker.stopWait();
	}

	void WorkerThread::add(Fn<void, Control&> &toCall) {
		Lock::L l(workLock);

		workList.push_back(toCall);
		waitSema.up();

		if (!worker.isRunning()) {
			worker.start(memberFn(this, &WorkerThread::threadMain));
		}
	}

	bool WorkerThread::isRunning() {
		return workList.size() > 0;
	}

	void WorkerThread::threadMain(Thread::Control &control) {
		Control ctrl(control);

		while (control.run()) {
			// Will block until another item is in the list
			waitSema.down();

			Fn<void, Control&> toCall;
			{
				Lock::L l(workLock);
				if (workList.size() > 0) {
					toCall = workList.front();
					workList.pop_front();
				} else {
					// Time to exit!
					return;
				}
			}

			toCall(ctrl);
		}
	}

}
