#pragma once
#include "Semaphore.h"

/**
 * Simple implementation of a condition variable. This implementation
 * is based on the assumption that only one thread will be waiting for
 * the condition.
 *
 * The condition variable will initially be blocking until someone calls
 * 'signal'. Then any current calls to 'wait' will be unblocked. If no thread
 * is blocked, the next call will not be blocked. What signal means is that
 * one call to 'wait' will be unblocked. More than one 'signal' call does
 * not matter. One artifact of this scheme is that 'wait' may return one time
 * too many in some cases.
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
	// Is the semaphore signaled?
	nat signaled;

	// Waitable semaphore.
	Semaphore sema;
};
