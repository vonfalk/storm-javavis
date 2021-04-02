#include "stdafx.h"
#include "Sync.h"
#include "Thread.h"
#include "UThread.h"

namespace os {

	Sema::Sema(nat count) : count(count) {}

	Sema::~Sema() {}

	void Sema::up() {
		UThreadData *toWake = null;

		{
			util::Lock::L z(lock);

			toWake = waiting.pop();
			if (!toWake) {
				// No thread to wake.
				count++;
				return;
			}
		}

		// Wake the thread up. Note: We can not do that inside the lock. Otherwise, the thread might
		// wake up before we have time to release the lock, which could cause us to crash if this
		// semaphore was deallocated by the thread that we woke (a typical use case for semaphores).
		toWake->owner->wake(toWake);
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

	/**
	 * Event.
	 */

	Event::Event() : s(0) {}

	Event::~Event() {
		set();
	}

	void Event::set() {
		atomicWrite(s, 1);

		// Wake all threads.
		util::Lock::L z(lock);
		while (UThreadData *data = waiting.pop()) {
			// Wake the thread up.
			UThreadState *state = data->owner;
			state->wake(data);
		}
	}

	void Event::clear() {
		atomicWrite(s, 0);
	}

	bool Event::isSet() {
		return atomicRead(s) == 1;
	}

	void Event::wait() {
		if (atomicRead(s) == 1)
			return;

		// Wait...

		UThreadState *state = null;
		{
			util::Lock::L z(lock);
			// Add us to the waiting queue.
			state = UThreadState::current();
			waiting.push(state->runningThread());
		}

		// Wait until we're woken up again.
		state->wait();
	}

}
