#pragma once
#include "Thread.h"
#include "Sync.h"

namespace os {

	class ThreadGroupData;

	/**
	 * Thread group. A thread group allows the owner of the thread group to wait until all members
	 * of that group have terminated. Beware to not add threads when calling join(), since there
	 * will be a race condition between the newly created thread and the terminating threads, which
	 * means that the thread group may not be empty after a 'join'.
	 */
	class ThreadGroup {
		friend class Thread;
	public:
		// Create.
		ThreadGroup();

		// Copy.
		ThreadGroup(const ThreadGroup &o);

		// Destroy.
		~ThreadGroup();

		// Assign.
		ThreadGroup &operator =(const ThreadGroup &o);

		// Wait for all threads in this group to exit. Only call from one thread!
		void join();

	private:
		// Data object we're referring.
		ThreadGroupData *data;
	};


	/**
	 * Shared data for thread group objects.
	 */
	class ThreadGroupData : NoCopy {
	public:
		// Create, initialize to one reference.
		ThreadGroupData();
		~ThreadGroupData();

		// Add reference.
		inline void addRef() {
			atomicIncrement(references);
		}

		// Release.
		inline void release() {
			if (atomicDecrement(references) == 0)
				delete this;
		}

		// Notify a new thread has started.
		void threadStarted();

		// Notify a thread is terminated.
		void threadTerminated();

		// Wait for all threads to terminate.
		void join();

	private:
		// Number of references.
		nat references;

		// Number of live threads. This is synced using atomic operations.
		nat attached;

		// Semaphore for waiting.
		Sema sema;
	};

}
