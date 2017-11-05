#include "stdafx.h"
#include "Semaphore.h"

#ifdef WINDOWS

Semaphore::Semaphore(long count, long maxCount) {
	semaphore = CreateSemaphore(NULL, count, max(count, maxCount), NULL);
}

Semaphore::~Semaphore() {
	CloseHandle(semaphore);
}

void Semaphore::up() {
	ReleaseSemaphore(semaphore, 1, NULL);
}

void Semaphore::down() {
	WaitForSingleObject(semaphore, INFINITE);
}

bool Semaphore::down(nat msTimeout) {
	DWORD result = WaitForSingleObject(semaphore, msTimeout);
	return result != WAIT_TIMEOUT;
}

#endif

#ifdef POSIX

Semaphore::Semaphore(long count, long maxCount) {
	UNUSED(maxCount);
	sem_init(&semaphore, 0, count);
}

Semaphore::~Semaphore() {
	sem_destroy(&semaphore);
}

void Semaphore::up() {
	sem_post(&semaphore);
}

void Semaphore::down() {
	while (sem_wait(&semaphore) != 0) {
		// We failed for some reason, probably a signal from the MPS.
		if (errno != EINTR) {
			perror("Waiting for a semaphore");
			std::terminate();
		}
	}
}

static void fillTimespec(struct timespec *out, nat ms) {
	clock_gettime(CLOCK_MONOTONIC, out);

	// Increase with 'ms'...
	const int64 maxNs = 1000LL * 1000LL * 1000LL;
	int64 ns = ms * 1000LL * 1000LL;
	ns += out->tv_nsec;

	out->tv_sec += ns / maxNs;
	out->tv_nsec = ns % maxNs;
}

bool Semaphore::down(nat msTimeout) {
	struct timespec time = {0, 0};
	fillTimespec(&time, msTimeout);

	while (sem_timedwait(&semaphore, &time) != 0) {
		// Failed for some reason...
		switch (errno) {
		case EINTR:
			// Just a signal from the MPS. Retry!
			// TODO: Adjust the time!
			break;
		case ETIMEDOUT:
			// We timed out. That means we're done and should abort!
			return false;
		default:
			perror("Waiting for a  semaphore with a timeout");
			std::terminate();
		}
	}

	// If we get here, we managed to acquire the semaphore!
	return true;
}

#endif
