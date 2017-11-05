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

	bool Condition::wait(nat msTimeout) {
		DWORD result = WaitForSingleObject(sema, msTimeout);
		if (result == WAIT_OBJECT_0) {
			atomicCAS(signaled, 1, 0);
			return true;
		} else {
			return false;
		}
	}

#endif

#ifdef POSIX

	Condition::Condition() : signaled(0) {
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
		if (atomicCAS(signaled, 0, 1) == 0)
			pthread_cond_signal(&cond);
	}

	void Condition::wait() {
		pthread_mutex_lock(&mutex);
		if (atomicCAS(signaled, 1, 0) == 0)
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
		int r = 0;

		pthread_mutex_lock(&mutex);
		if (atomicCAS(signaled, 1, 0) == 0)
			r = pthread_cond_timedwait(&cond, &mutex, &time);
		pthread_mutex_unlock(&mutex);

		if (r == 0)
			return true;
		else
			return errno != ETIMEDOUT;
	}

#endif

}
