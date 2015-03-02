#include "stdafx.h"
#include "Sync.h"

namespace code {

	Sema::Sema(nat count) : count(count) {}

	Sema::~Sema() {}

	void Sema::up() {
		::Lock::L z(lock);

		UThreadData *data = waiting.pop();
		if (data)
			; // wake the thread
		else
			count++;
	}

	void Sema::down() {
		while (true) {
			{
				::Lock::L z(lock);
				if (count > 0) {
					count--;
					return;
				}
			}

			// wait...
			UThread::leave();

			TODO(L"Implement a real wait here!");
		}
	}


	/**
	 * Lock
	 */

	Lock::Lock() {}

	Lock::~Lock() {}

	Lock::L::L(Lock &l) : owner(l) {
		owner.lock();
	}

	Lock::L::~L() {
		owner.unlock();
	}

	void Lock::lock() {
		assert(false, L"Not implemented yet!");
	}

	void Lock::unlock() {
		assert(false, L"Not implemented yet");
	}

}
