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
		pthread_mutex_init(&cs, null);
	}

	Lock::~Lock() {
		pthread_mutex_destroy(&cs);
	}

	Lock::L::L(Lock &l) : l(&l) {
		pthread_mutex_lock(&l.cs);
	}

	Lock::L::L(Lock *l) : l(l) {
		if (l)
			pthread_mutex_lock(&l->cs);
	}

	Lock::L::~L() {
		if (l)
			pthread_mutex_unlock(&l->cs);
	}

#endif

}
