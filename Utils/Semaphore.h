#pragma once

#include "Windows.h"

class Semaphore {
public:
	//Creates a semaphore with an initial count of "count". Assumes that
	//count will never reach above the initial "count". Count shall not be negative.
	Semaphore(long count = 1, long maxCount = 1);
	~Semaphore();

	void up();
	void down();
private:
	HANDLE semaphore;
};

