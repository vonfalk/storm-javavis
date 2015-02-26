#include "stdafx.h"
#include "Condition.h"

Condition::Condition() : sema(0) {}

void Condition::signal() {
	TODO("Test the Condition variable!");
	// If someone was waiting, report that we will up the semaphore.
	if (atomicCAS(waiting, 1, 0) == 1)
		sema.up();
}

void Condition::wait() {
	TODO("Test the Condition variable!");
	atomicCAS(waiting, 0, 1);
	sema.down();
}
