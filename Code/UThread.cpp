#include "stdafx.h"
#include "UThread.h"
#include "Utils/Math.h"

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

	// No need to deallocate the current stack.
	static THREAD Stack currentStack = { 0, 0 };

	// Next thread to schedule (null = none, we're alone).
	static THREAD UThread *nextThread = null;

	// Thread to remove at earliest opportunity.
	static THREAD UThread *terminatedThread = null;


	UThread::UThread() : next(null), prev(null) {
		static nat threadId = 1;
		id = threadId++;

		stack = allocStack();
		esp = initialEsp();
	}

	UThread::~UThread() {
		freeStack(stack);
	}

	void UThread::spawn(const Fn<void, void> &fn) {
		UThread *t = new UThread();

		t->initialStack(address(&UThread::main), new Fn<void, void>(fn));

		// Add to the back of the queue.
		if (UThread *next = nextThread) {
			t->prev = next->prev;
			t->next = next;

			t->prev->next = t;
			t->next->prev = t;
		} else {
			t->next = t;
			t->prev = t;
			nextThread = t;
		}
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

		// It should not be possible to exit from the 'main' thread of this OS thread.
		assert(nextThread != null);

		// Remove the running thread, and schedule the next one instead.
		UThread *t = nextThread;
		terminatedThread = t;

		if (t->next == t) {
			// We're alone!
			nextThread = null;
		} else {
			t->next->prev = t->prev;
			t->prev->next = t->next;
			nextThread = t->next;
		}

		t->switchTo();

		// Should never get here!
		assert(false);
	}

	bool UThread::any() {
		return nextThread != null;
	}

	void UThread::leave() {
		if (!any())
			return;

		// Go to the next thread.
		UThread *t = nextThread;
		nextThread = t->next;

		// Careful with this! It does _not_ behave as the compiler
		// thinks it does, better keep it late!
		t->switchTo();

		// One possible exit of 'switchTo'
		afterSwitch();
	}

	void UThread::afterSwitch() {
		reap();
	}

	void UThread::reap() {
		if (terminatedThread) {
			delete terminatedThread;
			terminatedThread = null;
		}
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
		swap(currentStack, stack);

		doSwitch(newEsp, &esp); // May not return.
	}

	void UThread::initialStack(void *fn, void *param) {
		push(param); // parameter
		push(0); // return to
		push(fn); // return to
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
