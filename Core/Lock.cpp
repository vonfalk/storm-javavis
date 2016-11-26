#include "stdafx.h"
#include "Lock.h"

namespace storm {

	Lock::Lock() : alloc(new Data()) {}

	Lock::Lock(const Lock &o) : alloc(o.alloc) {
		atomicIncrement(alloc->refs);
	}

	Lock::~Lock() {
		if (atomicDecrement(alloc->refs) == 0) {
			// Last one remaining!
			delete alloc;
		}
	}

	void Lock::deepCopy(CloneEnv *) {
		// Nothing to do.
	}

	void Lock::lock() {
		size_t owner = (size_t)os::UThread::current().threadData();
		if (atomicRead(alloc->owner) == owner) {
			// We've already locked this lock.
			alloc->recursion++;
		} else {
			// That was not us. Lock properly.
			alloc->sema.down();
			alloc->owner = owner;
			alloc->recursion = 1;
		}
	}

	void Lock::unlock() {
		size_t owner = (size_t)os::UThread::current().threadData();
		assert(owner == alloc->owner, L"Attempting to unlock from wrong thread!");

		if (--alloc->recursion == 0) {
			alloc->owner = 0;
			alloc->sema.up();
		}
	}

	Lock::L::L(Lock *l) : lock(l) {
		lock->lock();
	}

	Lock::L::~L() {
		lock->unlock();
	}

	Lock::Data::Data() : refs(1), owner(0), recursion(0), sema(1) {}

}
