#pragma once
#include "UThread.h"
#include "Utils/Lock.h"

namespace os {

	class UThread;
	class UThreadData;

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
		util::Lock lock;
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

		// Semaphore for locking features, we do not yet check ownership of
		// the lock, that is handled fairly good by the L class.
		// TODO: Maybe recursive locking as well?
		Sema sema;
	};


	/**
	 * Event. The event starts cleared, which means that calls to 'wait' will block until the event
	 * is 'set'.
	 */
	class Event : NoCopy {
	public:
		// Create.
		Event();

		// Destroy. Sets the event to not loose any threads.
		~Event();

		// Set.
		void set();

		// Clear.
		void clear();

		// Check set or cleared.
		bool isSet();

		// Wait.
		void wait();

	private:
		// Lock for ourselves.
		util::Lock lock;

		// Waiting threads.
		InlineList<UThreadData> waiting;

		// Is the event set?
		nat s;
	};

}
