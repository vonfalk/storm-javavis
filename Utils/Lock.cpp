#include "stdafx.h"
#include "Lock.h"

namespace util {

#ifdef WINDOWS
	Lock::Lock() {
		InitializeCriticalSection(&cs);
	}

	Lock::~Lock() {
		DeleteCriticalSection(&cs);
	}

	Lock::L::L(Lock &l) : l(&l) {
		EnterCriticalSection(&l.cs);
	}

	Lock::L::L(Lock *l) : l(l) {
		if (l)
			EnterCriticalSection(&l->cs);
	}

	Lock::L::~L() {
		if (l)
			LeaveCriticalSection(&l->cs);
	}
#endif

#ifdef POSIX
	Lock::Lock() {
		pthread_mutexattr_t attr;
		pthread_mutexattr_init(&attr);
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
		pthread_mutex_init(&cs, &attr);
		pthread_mutexattr_destroy(&attr);
	}

	Lock::~Lock() {
		pthread_mutex_destroy(&cs);
	}

	Lock::L::L(Lock &l) : l(&l) {
		// According to the man pages, we do not need to consider EINTR from locking a mutex.
		pthread_mutex_lock(&l.cs);
	}

	Lock::L::L(Lock *l) : l(l) {
		// According to the man pages, we do not need to consider EINTR from locking a mutex.
		if (l)
			pthread_mutex_lock(&l->cs);
	}

	Lock::L::~L() {
		if (l)
			pthread_mutex_unlock(&l->cs);
	}

#endif

}
