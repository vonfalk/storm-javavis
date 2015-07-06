#include "stdafx.h"
#include "Condition.h"

Condition::Condition() : signaled(0), sema(0) {}

void Condition::signal() {
	// If we're the first one to signal, alter the semaphore.
	if (atomicCAS(signaled, 0, 1) == 0)
		sema.up();
}

void Condition::wait() {
	// Wait for someone to signal, and then reset the signaled state for next time.
	sema.down();
	atomicCAS(signaled, 1, 0);
}

bool Condition::wait(nat msTimeout) {
	bool r = sema.down(msTimeout);
	if (r)
		atomicCAS(signaled, 1, 0);
	return r;
}
