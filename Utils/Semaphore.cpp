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

bool Semaphore::down(nat msTimeout) {
	assert(false, L"Can not do sema_down with a timeout at the moment!");
	UNUSED(msTimeout);
	return false;
}

#endif
