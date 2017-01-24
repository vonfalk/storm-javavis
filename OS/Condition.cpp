#include "stdafx.h"
#include "Condition.h"

namespace os {

#ifdef WINDOWS

	Condition::Condition() : signaled(0) {
		sema = CreateSemaphore(NULL, 0, 1, NULL);
	}

	Condition::~Condition() {
		CloseHandle(sema);
	}

	void Condition::signal() {
		// If we're the first one to signal, alter the semaphore.
		if (atomicCAS(signaled, 0, 1) == 0)
			ReleaseSemaphore(sema, 1, NULL);
	}

	void Condition::wait() {
		// Wait for someone to signal, and then reset the signaled state for next time.
		WaitForSingleObject(sema, INFINITE);
		atomicCAS(signaled, 1, 0);
	}

	void Condition::wait(IOHandle io) {
		HANDLE handles[2] = { sema, io.v() };
		WaitForMultipleObjects(io ? 2 : 1, handles, FALSE, INFINITE);
		atomicCAS(signaled, 1, 0);
	}

	bool Condition::wait(nat msTimeout) {
		DWORD result = WaitForSingleObject(sema, msTimeout);
		if (result == WAIT_OBJECT_0) {
			atomicCAS(signaled, 1, 0);
			return true;
		} else {
			return false;
		}
	}

	bool Condition::wait(IOHandle io, nat msTimeout) {
		HANDLE handles[2] = { sema, io.v() };
		DWORD result = WaitForMultipleObjects(io ? 2 : 1, handles, FALSE, msTimeout);
		if (result == WAIT_OBJECT_0 || result == WAIT_OBJECT_0+1) {
			atomicCAS(signaled, 1, 0);
			return true;
		} else {
			return false;
		}
	}

#else
#error "Implemend Condition!"
#endif

}
