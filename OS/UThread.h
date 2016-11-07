#pragma once
#include "FnParams.h"
#include "InlineList.h"
#include "Utils/InlineSet.h"
#include "Utils/Function.h"
#include "Utils/Lock.h"
#include "Future.h"

namespace os {

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

		// Yeild for a specific amount of time.
		static void sleep(nat ms);

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
		static UThread spawn(const util::Fn<void, void> &fn, const Thread *on = null);

		// Spawn using a plain function pointer and parameters. The parameters stored in
		// 'params' follows the same lifetime rules as FnParams::call() does. No special care
		// needs to be taken. Note: fn may not return a value!
		static UThread spawn(const void *fn, bool memberFn, const FnParams &params,
							const Thread *on = null, UThreadData *prealloc = null);

		// Spawn a thread, returning the result in a future. Keep the Future object alive until
		// it has gotten a result, otherwise we will probably crash! This is the low-level variant.
		// It may also be used from the 'spawnLater' api by setting 'prealloc' to something other than null.
		static UThread spawn(const void *fn, bool memberFn, const FnParams &params, FutureBase &result,
							const BasicTypeInfo &resultType, const Thread *on = null, UThreadData *prealloc = null);

		// Spawn using a plain function pointer and parameters. Places the result (including any exceptions)
		// in the future object.
		template <class R, class Sema>
		static UThread spawn(const void *fn, bool memberFn, const FnParams &params,
							Future<R, Sema> &future, const Thread *on = null) {
			return spawn(fn, memberFn, params, future.impl(), typeInfo<R>(), on);
		}

		/**
		 * Spawn a new UThread in two step. This can be used to pass parameters easily from
		 * machine code, without allocating any memory at all except for the stack and internal
		 * data needed by the UThread itself (which may be removed in the future).
		 * This is a four step process:
		 * 1: call spawnLater to allocate the thread itself.
		 * 2: call spawnParamMem(v) to get a pointer to the memory usable for parameters.
		 * 3: add parameters using FnParams
		 * 4: call either UThread::spawn() or abortSpawn()
		 */

		// Spawn a thread later. Returns a thread that is allocated but not yet scheduled. Make
		// sure to either start it using 'spawn' or 'abortSpawn' to avoid leaking resources.
		static UThreadData *CODECALL spawnLater();

		// Get memory usable for parameters.
		static void *CODECALL spawnParamMem(UThreadData *data);

		// Abort the spawn.
		static void CODECALL abortSpawn(UThreadData *data);


		// Get the thread data. Mainly for internal use.
		inline UThreadData *threadData() { return data; }

		// Value representing no thread.
		static const UThread invalid;

	private:
		// Create
		UThread(UThreadData *data);

		// Referenced data.
		UThreadData *data;

		// Insert an UThread.
		static UThread insert(UThreadData *data, const Thread *on);
	};


	/**
	 * Stack description for an UThread. This is exposed so that we can garbage collect them
	 * properly.
	 */
	class UThreadStack : public util::SetMember<UThreadStack> {
	public:
		UThreadStack();

		// Description.
		struct Desc {
			// Information about the current stack. 'high' is the currently highest used address
			// (inclusive) and 'low' is the lowest used address (exclusive).
			void *low;
			void *dummy;
			void *high;
		};

		// Current stack description. If null, then this UThread is currently running, and the
		// current CPU state describes the stack of that thread.
		Desc *desc;
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

		// A description of the current stack for this UThread.
		UThreadStack stack;

		// The stack we allocated for this UThread. May be null.
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
		void pushParams(const void *returnTo, void *param1, void *param2);
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
	 * This class is not threadsafe, except where noted.
	 */
	class UThreadState : NoCopy {
	public:
		// Create.
		UThreadState(ThreadData *owner);

		// Destroy.
		~UThreadState();

		// The thread we belong to.
		ThreadData *const owner;

		// List of stacks for all UThreads running on this hardware thread. This includes any
		// threads not on the ready-queue, and allows garbage collecting the UThreads.
		// Protected by the same lock as the Ready-queue.
		util::InlineSet<UThreadStack> stacks;

		// Get the state for the current thread.
		static UThreadState *current();

		// Any more ready threads? This includes waiting threads.
		bool any();

		// Schedule the next thread.
		bool leave();

		// Sleep.
		void sleep(nat ms);

		// Exits from the current thread and does not schedule it until
		// it is 'inserted' again. Make sure to call 'wake' on the current thread later,
		// otherwise we will leak memory.
		void wait();

		// Wake up a sleeping thread. Only works for threads which have been 'wait'ed earlier.
		void wake(UThreadData *data);

		// Exit the current thread.
		void exit();

		// Add a new thread as 'ready'. Safe to call from other OS threads.
		// Note: make sure to add a reference to the thread before calling insert, otherwise
		// it may be deleted before 'insert' returns.
		void insert(UThreadData *data);

		// Get the currently running thread.
		inline UThreadData *runningThread() { return running; }

	private:
		// Currently running thread here.
		UThreadData *running;

		// Lock for the 'ready' list and the 'stacks' set. The 'exit' list is not protected, since
		// it is only ever accessed from the OS thread owning this state.
		util::Lock lock;

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


// Don't ask...
#include "Sync.h"
