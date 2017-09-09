#pragma once
#include "FnCall.h"
#include "InlineList.h"
#include "SortedInlineList.h"
#include "InlineSet.h"
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

		// Yield to another UThread. Returns sometime in the future. Returns 'true' if other
		// threads were run between the call and the return.
		static bool leave();

		// Yield for a specific amount of time.
		static void sleep(nat ms);

		// Any more UThreads to run here?
		static bool any();

		// Get the currently running UThread.
		static UThread current();

		// Get the thread data. Mainly for internal use.
		inline UThreadData *threadData() { return data; }

		// Value representing no thread.
		static const UThread invalid;


		/**
		 * Low-level spawn functions. See 'spawn' below for explanations.
		 */

		// Spawn a 'void' function.
		static UThread spawnRaw(const void *fn, bool memberFn, void *firstParam,
								const FnCallRaw &call, const Thread *on = null);

		// Spawn a function, capturing the result in a future.
		static UThread spawnRaw(const void *fn, bool memberFn, void *firstParam,
								const FnCallRaw &call, FutureBase &result,
								void *target, const Thread *on = null);


		/**
		 * Spawn a new UThread on the currently running thread, or another thread. There are a few
		 * variants of spawn here, all of them take some kind of function pointer, optionally with
		 * parameters to run, followed by a reference to the OS thread (Thread *) to run on. If this
		 * is left out, or set to null, the thread of the caller is used.
		 *
		 * All of these return as soon as the new UThread is set up, they do not pre-empt the
		 * currently running thread. Note, however, that any parameters are copied before the call
		 * returns, so there is no need to worry about variables used as parameters going out of
		 * scope.
		 */

		// Spawn using a Fn<void, void>.
		static UThread spawn(const util::Fn<void, void> &fn, const Thread *on = null);

		// Spawn using a FnCall object.
		template <int P>
		static UThread spawn(const void *fn, bool memberFn, const FnCall<void, P> &call, const Thread *on = null) {
			return spawnRaw(fn, memberFn, null, call, on);
		}

		// Spawn using a FnCall object, providing the result in a Future<T>.
		template <class R, int P, class Sema>
		static UThread spawn(const void *fn, bool memberFn, const FnCall<R, P> &call,
							Future<R, Sema> &result, const Thread *on = null) {
			return spawnRaw(fn, memberFn, null, call, result.impl(), result.data(), on);
		}

	private:
		// Create
		UThread(UThreadData *data);

		// Referenced data.
		UThreadData *data;

		// Insert an UThread.
		static UThread insert(UThreadData *data, ThreadData *on);
	};


	/**
	 * Stack description for an UThread. This is exposed so that we can garbage collect them
	 * properly.
	 */
	class UThreadStack : public SetMember<UThreadStack> {
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

		// Stack bottom for the current thread. Only used if 'low' or 'high' is missing (depends on
		// the current architecture).
		void *stackLimit;
	};


	/**
	 * UThread data.
	 */
	class UThreadData : NoCopy {
		// Create.
		UThreadData(UThreadState *thread);

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
		static UThreadData *createFirst(UThreadState *thread, void *stackBase);

		// Create any other threads.
		static UThreadData *create(UThreadState *thread);

		// Destroy.
		~UThreadData();

		// A description of the current stack for this UThread.
		UThreadStack stack;

		// The stack we allocated for this UThread. May be null.
		void *stackBase;
		nat stackSize;

		// Move this UThread to another Thread.
		void move(UThreadState *from, UThreadState *to);

		// Switch from this thread to 'to'.
		void switchTo(UThreadData *to);

		// Push values to the stack.
		void push(void *v);
		void push(intptr_t v);
		void push(uintptr_t v);

		// Push the initial context to the stack. This contains where to 'return' to, and any
		// parameters that shall be passed to that function.
		void pushContext(const void *returnTo);
		void pushContext(const void *returnTo, void *param);
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
		UThreadState(ThreadData *owner, void *stackBase);

		// Destroy.
		~UThreadState();

		// The thread we belong to.
		ThreadData *const owner;

		// List of stacks for all UThreads running on this hardware thread. This includes any
		// threads not on the ready-queue, and allows garbage collecting the UThreads.
		// Protected by the same lock as the Ready-queue.
		InlineSet<UThreadStack> stacks;

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

		// Return the time (in ms) until the next UThread shall wake. Returns false if no thread to wake.
		bool nextWake(nat &time);

		// Wake threads if neccessary.
		void wakeThreads();

		// Get the currently running thread.
		inline UThreadData *runningThread() { return running; }

		// Notify there is a new stack.
		void newStack(UThreadData *data);

		/**
		 * Take a detour to another thread for a while, with the intention to return directly to the
		 * currently running thread later. Used while spawning threads.
		 *
		 * Only a single detour can be active at any point.
		 *
		 * Do not take a detour to a thread that is already on the ready queue. It is fine if the
		 * thread being detoured to is associated with another Thread than the one represented by
		 * the current UThreadState.
		 */

		// Take a detour to another UThread. Returns whatever was passed as 'result' to 'endDetour'.
		void *startDetour(UThreadData *data);

		// End the detour, returning to the thread which called 'startDetour'.
		void endDetour(void *result = null);

	private:
		// Data for sleeping threads.
		struct SleepData {
			inline SleepData(int64 until) : next(null), until(until) {}

			// Next entry in the list.
			SleepData *next;

			// Wait until this timestamp.
			int64 until;

			// Signal wait done.
			virtual void signal() = 0;

			// Compare.
			inline bool operator <(const SleepData &o) const {
				return until < o.until;
			}
		};

		// Currently running thread here.
		UThreadData *running;

		// Lock for the 'ready' list and the 'stacks' set. The 'exit' list is not protected, since
		// it is only ever accessed from the OS thread owning this state.
		util::Lock lock;

		// Ready threads. May be scheduled now.
		InlineList<UThreadData> ready;

		// Keep track of exited threads. Remove these at earliest opportunity!
		InlineList<UThreadData> exited;

		// Threads which are currently waiting. Not protected by locks as it is only accessed from
		// the OS thread owning this state.
		SortedInlineList<SleepData> sleeping;

		// Number of threads alive. Always updated using atomics, no locks. Threads
		// that are waiting and not stored in the 'ready' queue are also counted.
		nat aliveCount;

		// Thread to return to after the current detour. 'null' if no current detour.
		UThreadData *detourOrigin;

		// Result from the detour.
		void *detourResult;

		// Elliminate any exited threads.
		void reap();

		// Wake threads up until 'timestamp'.
		void wakeThreads(int64 time);
	};

	/**
	 * Errors in the threading.
	 */
	class ThreadError : public Exception {
	public:
		ThreadError(const String &msg) : msg(msg) {}
		virtual String what() const { return msg; }
	private:
		String msg;
	};


}


// Don't ask...
#include "Sync.h"
