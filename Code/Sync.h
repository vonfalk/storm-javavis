#pragma once
#include "UThread.h"
#include "Utils/Lock.h"

namespace code {

	/**
	 * Synchronization primitives aware of UThreads.
	 */


	/**
	 * Semaphore.
	 */
	class Sema : NoCopy {
	public:
		// Create.
		Sema(nat count = 1);

		// Destroy, will unblock any waiting threads.
		~Sema();

		// Count up. Potentially releases a thread.
		void up();

		// Count down. Blocks until the count is above zero.
		void down();

	private:
		// The current count.
		nat count;

		// Threads waiting here.
		InlineList<UThreadData> waiting;

		// Lock the semaphore implementation.
		::Lock lock;
	};


	/**
	 * Simple lock. Works much like the one found in Utils.
	 */
	class Lock : NoCopy {
	public:
		// Create
		Lock();

		// Destroy.
		~Lock();

		// Locking the lock.
		class L {
		public:
			L(Lock &l);
			~L();

		private:
			L(const L &);
			L &operator =(const L &);

			Lock &owner;
		};

	private:
		// Lock and unlock manually.
		void lock();

		// Lock.
		void unlock();
	};

}
