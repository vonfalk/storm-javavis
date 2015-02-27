#include "stdafx.h"
#include "UThread.h"
#include "Thread.h"
#include "Utils/Math.h"
#include "Utils/Lock.h"

namespace code {

	// Approximate stack size. (we need about 20K to be able to do cout)
	static nat stackSize = 1024 * 40;

#ifdef WINDOWS

	static nat pageSize() {
		static nat sz = 0;
		if (sz == 0) {
			SYSTEM_INFO sysInfo;
			GetSystemInfo(&sysInfo);
			sz = sysInfo.dwPageSize;
		}
		return sz;
	}

	static nat allocSize() {
		static nat sz = 0;
		if (sz == 0) {
			nat p = pageSize();
			sz = roundUp(stackSize, p);
			sz += p; // for the guard page
		}
		return sz;
	}

	static Stack allocStack() {
		nat size = allocSize();
		void *memory = VirtualAlloc(null, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		DWORD oldProt;
		VirtualProtect(memory, pageSize() - 1, PAGE_READONLY | PAGE_GUARD, &oldProt);
		Stack s = { (byte *)memory, size };
		return s;
	}

	static void freeStack(Stack s) {
		if (s.top) {
			VirtualFree(s.top, 0, MEM_RELEASE);
		}
	}

#endif

	// The state data for UThreads.
	// All variables in here are only used from one thread, except the linked
	// list in 'newHead', 'newTail', and 'newCount'. 'newHead' and 'newTail'
	// are protected by 'newLock', while 'newCount' should be used atomically.
	struct UState {
		// The stack of the currently running thread. We do not need to
		// de-allocate the stack of the thread provided by the OS, so we
		// initialize it to (0, 0).
		Stack stack;

		// The next thread in a circularly linked queue.
		UThread *next;

		// Thread to remove as soon as possible.
		UThread *terminated;

		// New nodes. Note that only 'next' is used here.
		UThread *newHead, *newTail;
		nat newCount;
		Lock *newLock;

		// Insert/remove a thread from the linked list.
		void insert(UThread *t);
		void remove(UThread *t);

		// Add a new UThread to the new list.
		void insertNew(UThread *t);

		// Copy all new threads to the 'regular' list.
		void copyNew();
	};

	// The current state for this thread.
	static THREAD UState state = { { 0, 0 }, null, null, null, null, 0, null };

	// Get the state for any thread.
	static UState &uState(const Thread *t) {
		if (t == null)
			return state;

		ThreadData *d = t->threadData();
		return *d->uState;
	}

	// Insert a new thread.
	static void insert(UThread *t, const Thread *to) {
		UState &s = uState(to);
		if (to == null) {
			s.insert(t);
		} else {
			s.insertNew(t);
			// Signal the thread.
			to->threadData()->wakeCond.signal();
		}
	}

	void UState::insert(UThread *t) {
		if (next) {
			t->prev = next->prev;
			t->next = next;

			t->prev->next = t;
			t->next->prev = t;
		} else {
			t->next = t;
			t->prev = t;
			next = t;
		}
	}

	void UState::remove(UThread *t) {
		if (next == t) {
			// Removing head element...
			if (t->next == t)
				next = null;
			else
				next = t->next;
		}

		t->next->prev = t->prev;
		t->prev->next = t->next;
	}

	void UState::insertNew(UThread *t) {
		Lock::L z(newLock);

		if (newTail) {
			newTail->next = t;
			newTail = t;
		} else {
			newHead = newTail = t;
			t->next = null;
		}

		atomicIncrement(newCount);
	}

	void UState::copyNew() {
		// Anything to do?
		if (atomicCAS(newCount, 0, 0) == 0)
			return;

		Lock::L z(newLock);

		while (newHead) {
			UThread *t = newHead;
			newHead = newHead->next;
			insert(t);
		}

		newHead = newTail = null;
		// Safe to just set the count here, only one consumer and the produces uses the lock.
		newCount = 0;
	}

	/**
	 * UThread.
	 */

	UThread::UThread() : next(null), prev(null) {
		stack = allocStack();
		esp = initialEsp();
	}

	UThread::~UThread() {
		freeStack(stack);
	}

	void UThread::initOsThread(ThreadData *data) {
		data->uState = &state;
		// Safe, this is done before anyone knows about us.
		state.newLock = new Lock;
	}

	void UThread::destroyOsThread(ThreadData *data) {
		data->uState = null;
		// Safe, this is done when everyone else has forgotten about us.
		del(state.newLock);
	}

	void UThread::remove() {
		// Remove, and schedule the next thread.
		UThread *t = state.next;
		state.terminated = t;

		assert(t != null, "Can not remove the original UThread!");
		state.remove(t);

		// Switch threads, never return.
		t->switchTo();
		assert(false);
	}

	bool UThread::any() {
		state.copyNew();
		return state.next != null;
	}

	void UThread::leave() {
		if (!any())
			return;

		// Go to the next thread.
		UThread *t = state.next;
		state.next = t->next;

		// Careful with this! It does _not_ behave as the compiler
		// thinks it does, better keep it late in the function!
		t->switchTo();

		// One possible exit of 'switchTo'
		afterSwitch();
	}

	void UThread::afterSwitch() {
		reap();
	}

	void UThread::reap() {
		if (state.terminated) {
			delete state.terminated;
			state.terminated = null;
		}
	}

	/**
	 * Spawn a thread running a Fn<void, void>:
	 */

	void UThread::spawn(const Fn<void, void> &fn, const Thread *on) {
		UThread *t = new UThread();
		t->pushParams(null, new Fn<void, void>(fn));
		t->pushContext(address(&UThread::main));
		insert(t, on);
	}

	void UThread::main(Fn<void, void> *fn) {
		try {
			// The other possible exit of 'switchTo'
			afterSwitch();

			(*fn)();
		} catch (...) {
			// Not good, but we can not let it pass further!
			assert(false);
		}

		delete fn;

		remove();
	}

	/**
	 * Spawn a thread running a FnCall:
	 */

	void UThread::spawn(const void *fn, const FnCall &params, const Thread *on) {
		UThread *t = new UThread();
		t->pushParams(address(&UThread::remove), params);
		t->pushContext(fn);
		insert(t, on);
	}


	// Machine specific code here as far as possible:
#ifdef X86

	void **UThread::initialEsp() {
		return (void **)(stack.top + stack.size);
	}

	void UThread::push(void *v) {
		*--esp = v;
	}

	void UThread::push(uintptr_t v) {
		*--esp = (void *)v;
	}

	void UThread::push(intptr_t v) {
		*--esp = (void *)v;
	}

#pragma warning (disable: 4733)
	// May not return. *oldEsp is updated with the current esp.
	// Note: we need to keep fs:[4] and fs:[8] updated (stack begin and end) updated
	// for exceptions to work in our threads. These could be saved through the 'stack'
	// member in the UThread itself, but this is much easier and only uses 8 bytes on the
	// stack anyway.
	static NAKED void doSwitch(void *newEsp, void ***oldEsp) {
		__asm {
			// Prolog. Bonus: saves ebp!
			push ebp;
			mov ebp, esp;

			// Store parameters.
			mov eax, oldEsp;
			mov ecx, newEsp;

			// Store state.
			push ebx;
			push esi;
			push edi;
			push dword ptr fs:[0];
			push dword ptr fs:[4];
			push dword ptr fs:[8];

			// Swap stacks.
			mov [eax], esp;
			mov esp, ecx;

			// Restore state.
			pop dword ptr fs:[8];
			pop dword ptr fs:[4];
			pop dword ptr fs:[0];
			pop edi;
			pop esi;
			pop ebx;

			// Epilog. Bonus: restores ebp!
			pop ebp;
			ret;
		}
	}

	void UThread::switchTo() {
		void **newEsp = esp;
		swap(state.stack, stack);

		doSwitch(newEsp, &esp); // May not return.
	}

	void UThread::pushParams(const void *fn, void *param) {
		push(param);
		push((void *)fn);
	}

	void UThread::pushParams(const void *fn, const FnCall &params) {
		nat s = params.paramsSize();
		assert(s % 4 == 0);
		esp -= s / 4;
		params.copyParams(esp);
		push((void *)fn);
	}

	void UThread::pushContext(const void *fn) {
		push((void *)fn); // return to
		push(0); // ebp
		push(0); // ebx
		push(0); // esi
		push(0); // edi
		push(0xFFFFFFFF); // seh (end of list is indicated by -1)
		push(stack.top + stack.size); // stack base
		push(stack.top); // stack limit
	}

#endif


}
