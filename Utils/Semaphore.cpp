#include "StdAfx.h"
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

