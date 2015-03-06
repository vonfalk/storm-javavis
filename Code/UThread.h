#pragma once
#include "FnParams.h"
#include "InlineList.h"
#include "Utils/Function.h"
#include "Utils/Lock.h"

namespace code {

	class Thread;
	class ThreadData;

	/**
	 * Implementation of user-level threads.
	 *
	 * Many of these threads are run on a single OS thread. They are
	 * cooperatively scheduled by calling the UThread::leave() function.
	 * Since the OS are unaware of these threads, it is not possible to
	 * use the standard OS syncronization primitives in all cases. Since
	 * the standard synchronization primitives are not aware of these threads,
	 * they will block all UThreads running on the OS thread, and therefore
	 * possibly cause deadlocks and other unexpected results. Use the
	 * code::Lock and code::Sema instead. These are found in "Sync.h"
	 */

	class UThreadData;
	class UThreadState;

	/**
	 * This is a handle to a specific UThread. Currently, not many operations
	 * is supported on another UThread than the current. Therefore, the backing
	 * UThread is not kept after the UThread has terminated.
	 */
	class UThread {
	public:

		/**
		 * Description of how to handle the result from a function call on another
		 * thread. When the function completes, 'done' is called with its result.
		 * If an exception is thrown, 'error' is called with the exception instead.
		 * 'data' may be used to provide custom data to the callbacks.
		 */
		template <class R, class Param>
		struct Result {
			Param *data;
			void (*done)(Param *, R);
			void (*error)(Param *, const Exception &);
		};

		// Specific for void.
		template <class Param>
		struct Result<void, Param> {
			Param *data;
			void (*done)(Param *);
			void (*error)(Param *, const Exception &);
		};


		/**
		 * Raw parameters to a spawn-call. Use 'result' above if possible to maintain
		 * some type-safety.
		 */
		struct Params {
			// Function to call.
			const void *toCall;

			// Data to the callbacks.
			void *data;

			// Callbacks.
			const void *onError;
			const void *onDone;

			// Result type info.
			TypeInfo result;
		};



		// Copy.
		UThread(const UThread &o);

		// Assign.
		UThread &operator =(const UThread &o);

		// Destroy.
		~UThread();

		// Same UThread?
		inline bool operator ==(const UThread &o) const { return data == o.data; }
		inline bool operator !=(const UThread &o) const { return data != o.data; }

		// Yeild to another UThread. Returns sometime in the future. Returns 'true' if other
		// threads were run between the call and the return.
		static bool leave();

		// Any more UThreads to run here?
		static bool any();

		// Get the currently running UThread.
		static UThread current();

		/**
		 * Spawn a new UThread on the currently running thread, or another thread.
		 * There are a few variants of spawn here, all of them take some kind of
		 * function pointer, optionally with parameters to run, followed by a reference
		 * to the OS thread (Thread *) to run on. If this is left out, or set to null,
		 * the thread of the caller is used.
		 * All of these return as soon as the new UThread is set up, they do not
		 * pre-empt the currently running thread.
		 */

		// Spawn using a Fn<void, void>.
		static UThread spawn(const Fn<void, void> &fn, const Thread *on = null);

		// Spawn using a plain function pointer and parameters. The parameters stored in
		// 'params' follows the same lifetime rules as FnParams::call() does. No special care
		// needs to be taken. Note: fn may not return a value!
		static UThread spawn(const void *fn, const FnParams &params, const Thread *on = null);

		// Spawn using a plain function pointer and parameters. Works much like the one below,
		// but is not as type-safe.
		static UThread spawn(const Params &p, const FnParams &params, const Thread *on = null);

		// Spawn using a plain function pointer and parameters. Calls either 'done' or 'error'
		// in 'result' when the execution of 'fn' is finished.
		template <class R, class P>
		static UThread spawn(const void *fn, const FnParams &params, const Result<R, P> &r, const Thread *on = null) {
			Params p = {
				fn,
				r.data,
				(const void *)r.error,
				(const void *)r.done,
				typeInfo<R>(),
			};
			return spawn(p, params, on);
		}

		// Get the thread data. Mainly for internal use.
		inline UThreadData *threadData() { return data; }

	private:
		// Create
		UThread(UThreadData *data);

		// Referenced data.
		UThreadData *data;
	};


	/**
	 * UThread data.
	 */
	class UThreadData : NoCopy {
		// Create.
		UThreadData();

	public:
		// Number of references.
		nat references;

		// Next position in inlined lists.
		UThreadData *next;

		// Owner.
		UThreadState *owner;

		// Add refcount.
		inline void addRef() {
			atomicIncrement(references);
		}

		inline void release() {
			if (atomicDecrement(references) == 0)
				delete this;
		}

		// Create for the first thread (where the stack is allocated by OS).
		static UThreadData *createFirst();

		// Create any other threads.
		static UThreadData *create();

		// Destroy.
		~UThreadData();

		// The current stack pointer. This is not valid if the thread is running.
		void **esp;

		// The current stack (lowest address and size). May be null.
		void *stackBase;
		nat stackSize;

		// Switch from this thread to 'to'.
		void switchTo(UThreadData *to);

		// Push values to the stack.
		void push(void *v);
		void push(intptr_t v);
		void push(uintptr_t v);

		// Push some parameters on the stack.
		void pushParams(const void *returnTo, void *param);
		void pushParams(const void *returnTo, const FnParams &params);

		// Push parameters while leaving some space on the stack.
		void *pushParams(const FnParams &params, nat space);

		// Allocate some space on the stack.
		void *alloc(size_t size);

		// Push the initial context to the stack.
		void pushContext(const void *returnTo);

	};

	/**
	 * Thread-specific state of the scheduler. It is designed to avoid
	 * locks as far as possible, to ensure high performance in thread
	 * switching.
	 * This class is not threadsafe, unless where noted.
	 */
	class UThreadState : NoCopy {
	public:
		// Create.
		UThreadState(ThreadData *owner);

		// Destroy.
		~UThreadState();

		// The thread we belong to.
		ThreadData *const owner;

		// Get the state for the current thread.
		static UThreadState *current();

		// Any more ready threads? This includes waiting threads.
		bool any();

		// Schedule the next thread.
		bool leave();

		// Exits from the current thread and does not schedule it until
		// it is 'inserted' again. Make sure to call 'wake' on the current thread later,
		// otherwise we will leak memory.
		void wait();

		// Wake up a sleeping thread. Only works for threads which have been 'wait'ed earlier.
		void wake(UThreadData *data);

		// Exit the current thread.
		void exit();

		// Add a new thread as 'ready'. Safe to call from other OS threads.
		void insert(UThreadData *data);

		// Get the currently running thread.
		inline UThreadData *runningThread() { return running; }

	private:
		// Currently running thread here.
		UThreadData *running;

		// Lock for the 'ready' list. The 'exit' list is not protected,
		// since it is only ever accessed from the OS thread owning this state.
		Lock lock;

		// Ready threads. May be scheduled now.
		InlineList<UThreadData> ready;

		// Keep track of exited threads. Remove these at earliest opportunity!
		InlineList<UThreadData> exited;

		// Number of threads alive. Always updated using atomics, no locks. Threads
		// that are waiting and not stored in the 'ready' queue are also counted.
		nat aliveCount;

		// Elliminate any exited threads.
		void reap();

	};


}
