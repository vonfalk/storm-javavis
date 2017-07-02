#pragma once

#include "Win32.h"

class Semaphore {
public:
	//Creates a semaphore with an initial count of "count". Assumes that
	//count will never reach above the initial "count". Count shall not be negative.
	Semaphore(long count = 1, long maxCount = 1);
	~Semaphore();

	void up();
	void down();

	// Timeout. Returns true if the semaphore was lowered normally, false if the timeout passed.
	bool down(nat msTimeout);
private:
#if defined(WINDOWS)
	HANDLE semaphore;
#elif defined(POSIX)
	sem_t semaphore;
#else
#error "No semaphores for this platform yet!"
#endif
};

