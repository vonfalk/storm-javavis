#include "stdafx.h"
#include "Sync.h"
#include "Thread.h"
#include "UThread.h"

namespace os {

	Sema::Sema(nat count) : count(count) {}

	Sema::~Sema() {}

	void Sema::up() {
		util::Lock::L z(lock);

		UThreadData *data = waiting.pop();
		if (!data) {
			// No thread to wake.
			count++;
			return;
		}

		// Wake the thread up.
		UThreadState *state = data->owner;
		state->wake(data);
		// Wake the OS thread, in case it is waiting for this thread.
		state->owner->wakeCond.signal();
	}

	void Sema::down() {
		UThreadState *state = null;

		{
			util::Lock::L z(lock);
			if (count > 0) {
				count--;
				return;
			}

			// Add us to the waiting queue.
			if (!state)
				state = UThreadState::current();
			waiting.push(state->runningThread());
		}

		// Wait until we're woken up again.
		state->wait();
	}


	/**
	 * Lock
	 */

	Lock::Lock() : sema(1) {}

	Lock::~Lock() {}

	Lock::L::L(Lock &l) : owner(l) {
		owner.lock();
	}

	Lock::L::~L() {
		owner.unlock();
	}

	void Lock::lock() {
		sema.down();
	}

	void Lock::unlock() {
		sema.up();
	}

}
