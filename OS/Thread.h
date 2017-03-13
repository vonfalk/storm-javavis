#pragma once
#include "Utils/Function.h"
#include "Utils/Semaphore.h"
#include "Condition.h"
#include "IOHandle.h"
#include "UThread.h"

namespace os {

	// Internal data.
	class ThreadData;
	class ThreadStart;

	// Interface for interaction with OS-specific waits. See below.
	class ThreadWait;

	// Thread group.
	class ThreadGroup;

	/**
	 * OS thread.
	 *
	 * The Thread object represents a live instance of an OS thread. You can
	 * use the == operator to see if two threads are the same. The underlying OS
	 * thread will be kept running (even though it is not doing anything useful)
	 * as long as there are any Thread objects referring the thread.
	 *
	 * For windows:
	 * CoInitializeEx will be called for every thread created from this class. For
	 * other threads, please call initThread and cleanThread when before using the
	 * thread.
	 */
	class Thread {
	public:
		// Copy.
		Thread(const Thread &o);

		// Assign.
		Thread &operator =(const Thread &o);

		// Destroy.
		~Thread();

		// Compare.
		bool operator ==(const Thread &o) const;
		inline bool operator !=(const Thread &o) const { return !(*this == o); }

		// Get the thread data.
		inline ThreadData *threadData() const { return data; }

		// Get an unique identifier for this thread.
		inline uintptr_t id() const { return (uintptr_t)data; }

		// Attach a os handle to this thread.
		void attach(Handle h) const;

		// Get a list of UThreads running on this thread. Note that access to this list is not thread safe.
		const util::InlineSet<UThreadStack> &stacks() const;

		// Start a thread.
		static Thread spawn(const util::Fn<void, void> &start, ThreadGroup &group);

		// Start a thread with a specific 'ThreadWait' behaviour. Will take ownership of 'wait'.
		static Thread spawn(ThreadWait *wait, ThreadGroup &group);

		// Get the current thread.
		static Thread current();

		// Invalid thread.
		static Thread invalid;

		// Initialize a thread with any OS specific resources.
		static void initThread();
		static void cleanThread();

		friend wostream &operator <<(wostream &to, const Thread &o);
	private:
		// Create.
		explicit Thread(ThreadData *data);

		// Thread data.
		ThreadData *data;
	};

	wostream &operator <<(wostream &to, const Thread &o);

	/**
	 * Internal thread data.
	 */
	class ThreadData {
	public:
		// Number of references.
		nat references;

		// Data used for the UThreads. Make sure to always run constructors and destructors
		// from the thread this ThreadData is representing.
		UThreadState uState;

		// Create.
		ThreadData();

		// Destroy.
		~ThreadData();

		// Add refcount.
		inline void addRef() {
			atomicIncrement(references);
		}

		// Release.
		inline void release() {
			if (atomicDecrement(references) == 0)
				reportZero();
		}

		// Report that an UThread has been awoken and wants to be scheduled.
		void reportWake();

		// Wait for another UThread to be scheduled. Returns 'true' as long as the 'wait' structure is used.
		bool waitForWork();

		// Check if there is any IO completion we shall handle.
		void checkIo() const;

		// Attach a handle.
		inline void attach(Handle h) { ioComplete.add(h, this); }

		// Thread main function.
		static void threadMain(ThreadStart &start);

	private:
		// Condition variable for waking up the thread when there is more work to do
		// or when it is time to exit.
		Condition wakeCond;

		// Current wait behavior.
		ThreadWait *wait;

		// Handle indicating the completion of any async IO operations.
		IOHandle ioComplete;

		// Report zero references.
		void reportZero();
	};


	/**
	 * Thread wait logic. Implement this to interact better with other events from the OS for a
	 * specific thread. Spawning a thread using a ThreadWait interface lets you control how long the
	 * thread should be kept alive, and allows you to implement custom waiting conditions.
	 */
	class ThreadWait {
	public:
		// The destructor will always be executed in the thread that has been 'wait'ing on this object.
		virtual ~ThreadWait();

		// Called before any work is done, on the thread that will call wait later on. Note that the
		// constructor will probably _not_ run on the same thread as 'init' will be run on.
		virtual void init();

		// Called when the thread should wait for an event of some kind. This functions should
		// return either when 'signal' has been called, but may return in other cases as well.
		// The thread is kept alive until 'wait' returns false. At this point, 'wait' will not be
		// called any more, and the ThreadWait will eventually be destroyed.
		// The passed handle shall also be examined and the wait shall be aborted if that becomes signaling.
		virtual bool wait(IOHandle io) = 0;

		// Called when wait() should be called, but when a timeout is also present.
		virtual bool wait(IOHandle io, nat msTimeout) = 0;

		// Called to indicate that any thread held by 'wait' should be awoken. May be called from
		// any thread. Calls to 'signal' after the last call to 'wait' may occur.
		virtual void signal() = 0;

		// Called from the root UThread as per the regular round-robin fashion. Will not be called
		// after 'wait' has returned false. Default implementation does nothing.
		virtual void work();
	};

}
