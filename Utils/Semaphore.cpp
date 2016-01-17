#include "stdafx.h"
#include "Semaphore.h"

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
