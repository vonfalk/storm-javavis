#include "stdafx.h"
#include "Lock.h"

namespace util {
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
}
