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
		HANDLE ioHandle = io.v();
		HANDLE handles[2] = { sema, ioHandle };
		DWORD r = WaitForMultipleObjects(ioHandle ? 2 : 1, handles, FALSE, INFINITE);
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
		HANDLE ioHandle = io.v();
		HANDLE handles[2] = { sema, ioHandle };
		DWORD result = WaitForMultipleObjects(ioHandle ? 2 : 1, handles, FALSE, msTimeout);
		if (result == WAIT_OBJECT_0 || result == WAIT_OBJECT_0+1) {
			atomicCAS(signaled, 1, 0);
			return true;
		} else {
			return false;
		}
	}

#endif

#ifdef POSIX

	Condition::Condition() {
		pthread_condattr_t attr;
		pthread_condattr_init(&attr);
		pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
		pthread_cond_init(&cond, &attr);
		pthread_condattr_destroy(&attr);

		pthread_mutex_init(&mutex, null);
	}

	Condition::~Condition() {
		pthread_cond_destroy(&cond);
	}

	void Condition::signal() {
		// '_signal' should be enough, but better safe than sorry.
		pthread_cond_broadcast(&cond);
	}

	void Condition::wait() {
		pthread_mutex_lock(&mutex);
		pthread_cond_wait(&cond, &mutex);
		pthread_mutex_unlock(&mutex);
	}

	void Condition::wait(IOHandle io) {
		UNUSED(io);
		TODO(L"Properly handle waiting for IO!");
		pthread_mutex_lock(&mutex);
		pthread_cond_wait(&cond, &mutex);
		pthread_mutex_unlock(&mutex);
	}

	static long ns(long ms) {
		return ms * 1000000;
	}

	static struct timespec timeMs(nat ms) {
		struct timespec time = {0, 0};
		clock_gettime(CLOCK_MONOTONIC, &time);

		nat s = ms / 1000;
		ms = ms % 1000;

		time.tv_sec += s;
		time.tv_nsec += ns(ms);
		while (time.tv_nsec >= ns(1000)) {
			time.tv_nsec -= ns(1000);
			time.tv_sec += 1;
		}

		return time;
	}

	bool Condition::wait(nat msTimeout) {
		struct timespec time = timeMs(msTimeout);;

		pthread_mutex_lock(&mutex);
		int r = pthread_cond_timedwait(&cond, &mutex, &time);
		pthread_mutex_unlock(&mutex);

		if (r == 0)
			return true;
		else
			return errno != ETIMEDOUT;
	}

	bool Condition::wait(IOHandle io, nat msTimeout) {
		UNUSED(io);
		TODO(L"Propery handle waiting for IO!");
		return wait(msTimeout);
	}

#endif

}
