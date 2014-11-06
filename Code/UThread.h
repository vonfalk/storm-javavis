#pragma once
#include "Utils/Function.h"

namespace code {

	/**
	 * Describes the stack of a UThread.
	 */
	struct Stack {
		// The top address (what we've allocated). Null if nothing.
		byte *top;

		// The total size of the stack.
		nat size;
	};


	/**
	 * Represents a user-level thread. These threads do not preempt by themselves,
	 * you have to call UThread::leave() to do that.
	 *
	 * This class is designed to hold information about all UThreads _not_ running.
	 * This class holds the needed state for any non-running thread. The threads
	 * are scheduled one after another, therefore they are linked into a circular buffer.
	 */
	class UThread : NoCopy {
	public:
		// Create another thread running 'fn'. Does not pre-empt this thread.
		static void spawn(const Fn<void, void> &fn);

		// Schedule the next thread. This function will eventually return when all other
		// UThreads have been given the chance to run.
		static void leave();

		// More threads running?
		static bool any();

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

		// Debug ID
		nat id;

		// Push ptr-sized values onto stack.
		void push(void *v);
		void push(uintptr_t v);
		void push(intptr_t v);

		// Compute the initial esp from our stack.
		void **initialEsp();

		// Initialize the stack to call 'fn' with 'param'.
		void initialStack(void *fn, void *param);

		// Switch to this thread. (does not behave as the compiler thinks it does!)
		// Note: does not even return for all switches (until later).
		void switchTo();

		// Main function in new threads.
		static void main(Fn<void, void> *fn);

		// Delete any pending deletions.
		static void reap();

		// Called after a thread switch.
		static void afterSwitch();
	};

}
