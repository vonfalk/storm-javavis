#pragma once
#include "Utils/Function.h"
#include "Function.h"

namespace code {

	class Thread;
	class ThreadData;

	/**
	 * Describes the stack of a UThread. Intended as an internal data structure.
	 */
	struct Stack {
		// The top address (what we've allocated). Null if nothing.
		byte *top;

		// The total size of the stack.
		nat size;
	};

	/**
	 * The current state of all threads for the current OS thread.
	 */
	struct UState;

	/**
	 * Represents a user-level thread. These threads do not preempt by themselves,
	 * you have to call UThread::leave() to do that.
	 *
	 * This class is designed to hold information about all UThreads _not_ running.
	 * This class holds the needed state for any non-running thread. The threads
	 * are scheduled one after another, therefore they are linked into a circular buffer.
	 */
	class UThread : NoCopy {
		friend struct UState;
	public:
		// TODO: provide some way of detecting when a thread has terminated, and/or a future-like
		// object with its result.

		// Create another thread running 'fn'. Does not pre-empt this thread. Parameters are
		// copied before the return of the call, and no special care of the lifetime of the
		// parameters in 'params' needs to be taken.
		static void spawn(const void *fn, const FnCall &params, const Thread *on = null);

		// Create another thread running 'fn'. Does not pre-empt this thread.
		static void spawn(const Fn<void, void> &fn, const Thread *on = null);

		// Schedule the next thread. This function will eventually return when all other
		// UThreads have been given the chance to run.
		static void leave();

		// More threads running?
		static bool any();

		/**
		 * For internal use:
		 * These are automatically called from Code/Thread.cpp when needed. Take care that the
		 * thread referred needs to be the calling thread as well.
		 */

		// Initialize the current OS thread to be able to receive spawns from other threads. This
		// is automatically called whenever a Thread object is created for the current thread.
		static void initOsThread(ThreadData *data);

		// Destroy anything allocated from 'initOsThread'. This is also done automatically
		// when using threads from Code/Thread.h
		static void destroyOsThread(ThreadData *data);

	private:
		// Create a new object from a stack.
		UThread();

		// Clean up.
		~UThread();

		// Stack used by this thread.
		Stack stack;

		// Current stack top.
		void **esp;

		// Next and prev UThread to run. Linked in a circular list.
		UThread *next, *prev;

		// Push ptr-sized values onto stack.
		void push(void *v);
		void push(uintptr_t v);
		void push(intptr_t v);

		// Compute the initial esp from our stack.
		void **initialEsp();

		// Initialize the stack to call 'fn' with 'param'.
		void initialStack(void *fn, void *param);

		// Push the initial context onto the stack (should be done last).
		void pushContext(const void *returnTo);

		// Push parameters to a function call.
		void pushParams(const void *returnTo, void *param);
		void pushParams(const void *returnTo, const FnCall &params);

		// Switch to this thread. (does not behave as the compiler thinks it does!)
		// Note: does not even return for all switches (until later).
		void switchTo();

		// Remove the running UThread from this thread's linked list. Implicitly switches to the
		// next free thread.
		static void remove();

		// Main function in new threads.
		static void main(Fn<void, void> *fn);

		// Delete any pending deletions.
		static void reap();

		// Called after a thread switch.
		static void afterSwitch();
	};

}
