#pragma once
#include "IOHandle.h"

namespace os {

	/**
	 * Simple implementation of a condition variable. This implementation is based on the assumption
	 * that only one thread will be waiting for the condition.
	 *
	 * The condition variable will initially be blocking until someone calls 'signal'. Then any current
	 * calls to 'wait' will be unblocked. If no thread is blocked, the next call will not be
	 * blocked. What signal means is that one call to 'wait' will be unblocked. More than one 'signal'
	 * call does not matter. One artifact of this scheme is that 'wait' may return one time too many in
	 * some cases.
	 *
	 * Note: this implementation could cause sporadic wakeups since we allow IO completion routines to
	 * execute while the semaphore is blocked.
	 */
	class Condition {
	public:
		// Ctor.
		Condition();
		~Condition();

		// Signal the condition, wakes up the thread if there is one waiting,
		// otherwise does nothing.
		void signal();

		// Wait for someone to signal the condition.
		void wait();
		void wait(IOHandle abort);

		// Wait for someone to signal the condition or until the timeout has passed.
		// true = signaled, false = timeout
		bool wait(nat msTimeout);
		bool wait(IOHandle abort, nat msTimeout);

	private:
#if defined(WINDOWS)
		// Is the semaphore signaled?
		nat signaled;

		// Waitable semaphore.
		HANDLE sema;
#elif defined(POSIX)
		// Condition object.
		pthread_cond_t cond;

		// We need a mutex as well.
		pthread_mutex_t mutex;
#else
#error "Please implement the condition for your platform!"
#endif
	};


}
