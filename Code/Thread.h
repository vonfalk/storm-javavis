#pragma once
#include "Utils/Function.h"
#include "Utils/Semaphore.h"
#include "Utils/Condition.h"
#include "UThread.h"

namespace code {

	class UThreadState;

	// Internal data.
	class ThreadData;
	class ThreadStart;

	/**
	 * OS thread.
	 *
	 * The Thread object represents a live instance of an OS thread. You can
	 * use the == operator to see if two threads are the same. The underlying OS
	 * thread will be kept running (even though it is not doing anything useful)
	 * as long as there are any Thread objects referring the thread.
	 */
	class Thread : public Printable {
	public:
		// Copy.
		Thread(const Thread &o);

		// Assign.
		Thread &operator =(const Thread &o);

		// Destroy.
		~Thread();

		// Compare.
		bool operator ==(const Thread &o) const;

		// Get the thread data.
		inline ThreadData *threadData() const { return data; }

		// Start a thread.
		static Thread start(const Fn<void, void> &start);

		// Get the current thread.
		static Thread current();

		// Thread main function.
		static void threadMain(ThreadStart &start);

	protected:
		virtual void output(wostream &to) const;

	private:
		// Create.
		explicit Thread(ThreadData *data);

		// Thread data.
		ThreadData *data;
	};


	// Internal thread data.
	class ThreadData {
	public:
		// Number of references.
		nat references;

		// Condition variable for waking up the thread when there is more work to do
		// or when it is time to exit.
		Condition wakeCond;

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

	private:
		// Report zero references.
		void reportZero();
	};

}
