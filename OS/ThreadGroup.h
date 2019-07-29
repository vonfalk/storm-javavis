#pragma once
#include "Thread.h"
#include "Sync.h"
#include "Utils/Lock.h"
#include "Utils/Function.h"

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

		// Callbacks called on thread start or thread termination.
		typedef util::Fn<void> Callback;
		ThreadGroup(Callback start, Callback stop);

		// Copy.
		ThreadGroup(const ThreadGroup &o);

		// Destroy.
		~ThreadGroup();

		// Assign.
		ThreadGroup &operator =(const ThreadGroup &o);

		// Get a list of all threads currently in the group.
		vector<Thread> threads() const;

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
		ThreadGroupData(ThreadGroup::Callback start, ThreadGroup::Callback stop);
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
		void threadStarted(ThreadData *data);

		// Notify that a thread is about to terminate. This is called when the thread believes that
		// it is no longer reachable. This is not a final decision as we may hand out references to
		// it. If 'false' is returned, there is a reference to the thread again, and termination
		// shall be held off for a while.
		bool threadUnreachable(ThreadData *data);

		// Notify a thread is terminated. This is a final decision, and should be called after 'threadUnreachable'.
		void threadTerminated();

		// Get all threads in this group.
		vector<Thread> threads();

		// Wait for all threads to terminate.
		void join();

	private:
		// Number of references.
		nat references;

		// Total number of threads attached.
		nat attached;

		// All threads currently running. Used to be able to produce a list of threads in the thread group.
		InlineSet<ThreadData> running;

		// Lock for the thread set.
		util::Lock runningLock;

		// Semaphore for waiting.
		Sema sema;

		// Callbacks for thread creation/termination.
		ThreadGroup::Callback start;
		ThreadGroup::Callback stop;
	};

}
