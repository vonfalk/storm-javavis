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

		// Declares what to do when the function terminates in the other thread. All functions
		// here will be running on the same UThread as the function called.
		template <class R, class Param>
		struct Result {
			// Data to pass to all functions.
			Param *data;

			// Function called when the function has terminated with result Result.
			void (*done)(Param *, R);

			// Function called when the function terminates with an exception.
			void (*error)(Param *, const Exception &);
		};

		// Specialize Result for void results.
		template <class Param>
		struct Result<void, Param> {
			// Data.
			Param *data;

			// Success.
			void (*done)(Param *);

			// Failure.
			void (*error)(Param *, const Exception &);
		};

		// Create another thread running 'fn'. Does not pre-empt this thread. Parameters
		// are copied before this function returns, so no special care needs to be taken
		// in that regard.
		template <class R, class P>
		static void spawn(const void *fn, const FnCall &params, const Result<R, P> &r, const Thread *on = null) {
			SpawnParams p = {
				fn,
				r.data,
				(const void *)r.error,
				(const void *)r.done,
				typeInfo<R>(),
			};
			spawn(p, params, on);
		}

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

		// Allocate things on the stack.
		void *alloc(size_t size);

		// Compute the initial esp from our stack.
		void **initialEsp();

		// Initialize the stack to call 'fn' with 'param'.
		void initialStack(void *fn, void *param);

		// Push the initial context onto the stack (should be done last).
		void pushContext(const void *returnTo);

		// Push parameters to a function call.
		void pushParams(const void *returnTo, void *param);
		void pushParams(const void *returnTo, const FnCall &params);

		// Push parameters to a function call a bit higher up on the stack (at least minSpace bytes free).
		// Does not modify esp, it is up to the programmer to ensure that we do not overwrite these.
		void *pushParams(const FnCall &params, nat minSpace);

		// Switch to this thread. (does not behave as the compiler thinks it does!)
		// Note: does not even return for all switches (until later).
		void switchTo();

		// Cleanup parameters struct. Needs to be POD.
		struct SpawnParams {
			// Function to call.
			const void *toCall;

			// Data to the callbacks.
			void *data;

			// Callbacks on completion of 'toCall'.
			const void *onError;
			const void *onResult;

			// Result type of 'toCall'.
			TypeInfo result;
		};

		// Spawn a thread where some cleanup should be used.
		static void spawn(const SpawnParams &s, const FnCall &params, const Thread *on);

		// Main function in new threads using SpawnParams.
		static void mainParams(SpawnParams *s, void *params);

		// Main function in new threads.
		static void main(Fn<void, void> *fn);

		// Remove the running UThread from this thread's linked list. Implicitly switches to the
		// next free thread.
		static void remove();

		// Delete any pending deletions.
		static void reap();

		// Called after a thread switch.
		static void afterSwitch();

		// Some low-level function calls.
		static void doUserCall(SpawnParams *s, void *params);
		static void doFloatCall(SpawnParams *s, void *params);
		static void doDoubleCall(SpawnParams *s, void *params);
		static void doCall4(SpawnParams *s, void *params);
		static void doCall8(SpawnParams *s, void *params);
	};

}
