#pragma once
#include "Semaphore.h"

/**
 * Simple implementation of a condition variable. This implementation
 * is based on the assumption that only one thread will be waiting for
 * the condition (asserted run-time in debug builds).
 */
class Condition {
public:
	// Ctor.
	Condition();

	// Signal the condition, wakes up the thread if there is one waiting,
	// otherwise does nothing.
	void signal();

	// Wait for someone to signal the condition.
	void wait();

private:
	// Is the thread waiting? (0 or 1)
	nat waiting;

	// Waitable semaphore.
	Semaphore sema;
};
