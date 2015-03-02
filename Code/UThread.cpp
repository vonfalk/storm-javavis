#include "stdafx.h"
#include "UThread.h"
#include "Thread.h"
#include "Utils/Math.h"
#include "Utils/Lock.h"

namespace code {

	/**
	 * Forward declare machine specific functions at the bottom.
	 */

	// Stack size. (we need about 30k on Windows to do cout).
	static nat stackSize = 40 * 1024;

	// Switch the currently running threads. *oldEsp is set to the old esp.
	// This returns as another thread, which may mean that it returns to the
	// beginning of another function in the case of newly started UThreads.
	void doSwitch(void *newEsp, void ***oldEsp); // can not be static for some reason...

	// Allocate a new stack.
	static void *allocStack(nat size);

	// Free a previously allocated stack.
	static void freeStack(void *base, nat size);

	// Compute the initial esp for this architecture.
	static void **initialEsp(void *base, nat size);

	// Some low-level function calls.
	static void doUserCall(UThread::Params *s, void *params);
	static void doFloatCall(UThread::Params *s, void *params);
	static void doDoubleCall(UThread::Params *s, void *params);
	static void doCall4(UThread::Params *s, void *params);
	static void doCall8(UThread::Params *s, void *params);

	// Choose the right call function. Done to prevent the compiler from
	// thinking that doXxxCall does not throw exceptions...
	typedef void (*DoCallFn)(UThread::Params *, void *params);
	static DoCallFn chooseCall(const TypeInfo &info);


	/**
	 * UThread.
	 */

	// Insert on a specific thread.
	static void insert(UThreadData *data, const Thread *on) {
		ThreadData *d;
		if (on)
			d = on->threadData();
		else
			d = Thread::current().threadData();

		d->uState.insert(data);
		d->wakeCond.signal();
	}

	// End the current thread.
	static void exitUThread() {
		UThreadState::current()->exit();
	}

	UThread::UThread(UThreadData *data) : data(data) {
		data->addRef();
	}

	UThread::UThread(const UThread &o) : data(o.data) {
		data->addRef();
	}

	UThread &UThread::operator =(const UThread &o) {
		data->release();
		data = o.data;
		data->addRef();
		return *this;
	}

	UThread::~UThread() {
		data->release();
	}

	bool UThread::leave() {
		return UThreadState::current()->leave();
	}

	bool UThread::any() {
		return UThreadState::current()->any();
	}

	static void spawnFn(Fn<void, void> *fn) {
		try {
			(*fn)();
		} catch (...) {
			assert(false, L"Uncaught exception!");
		}

		delete fn;
		exitUThread();
	}

	static void spawnParams(UThread::Params *p, void *params) {
		// Note: Do not call anything strange before we call p->toCall, otherwise
		// we may overwrite parameters already on the stack!
		typedef void (*ErrorFn)(void *, const Exception &);

		try {
			// This is done to force VS2008 compiler to see that this function
			// may throw an exception and include exception handling here.
			DoCallFn fn = chooseCall(p->result);
			(*fn)(p, params);
		} catch (const Exception &e) {
			const ErrorFn f = (const ErrorFn)p->onError;
			(*f)(p->data, e);
		} catch (...) {
			// We can not throw any further.
			// should assert, but it takes too much stack space!
		}

		// Terminate ourselves.
		exitUThread();
	}

	UThread UThread::spawn(const Fn<void, void> &fn, const Thread *on) {
		UThreadData *t = UThreadData::create();

		t->pushParams(null, new Fn<void, void>(fn));
		t->pushContext(&spawnFn);

		insert(t, on);
		return UThread(t);
	}

	UThread UThread::spawn(const void *fn, const FnCall &params, const Thread *on) {
		UThreadData *t = UThreadData::create();

		t->pushParams(&exitUThread, params);
		t->pushContext(fn);

		insert(t, on);
		return UThread(t);
	}

	UThread UThread::spawn(const Params &p, const FnCall &params, const Thread *on) {
		UThreadData *t = UThreadData::create();

		// Copy parameters to the stack of the new thread.
		Params *pPos = (Params *)t->alloc(sizeof(p));
		*pPos = p;

		// Copy parameters a bit over the top. Space for a return value and some temporary space between.
		void *paramsPos = t->pushParams(params, p.result.size);

		// Set up the initial function call.
		t->pushParams(null, FnCall().param(pPos).param(paramsPos));
		t->pushContext(&spawnParams);

		// Done.
		insert(t, on);
		return UThread(t);
	}

	/**
	 * UThread data.
	 */

	UThreadData::UThreadData() : references(0), next(null), stackBase(null), stackSize(null), esp(null) {}

	UThreadData *UThreadData::createFirst() {
		// For the thread where the stack was allocated by the OS, we do not need to care.
		return new UThreadData();
	}

	UThreadData *UThreadData::create() {
		UThreadData *t = new UThreadData();
		t->stackSize = code::stackSize;
		t->stackBase = allocStack(t->stackSize);
		t->esp = initialEsp(t->stackBase, t->stackSize);
		return t;
	}

	UThreadData::~UThreadData() {
		if (stackBase)
			freeStack(stackBase, stackSize);
	}

	void UThreadData::switchTo(UThreadData *to) {
		doSwitch(to->esp, &esp);
	}

	/**
	 * UThread state.
	 */

	static THREAD UThreadState *threadState = null;

	UThreadState::UThreadState() {
		threadState = this;

		running = UThreadData::createFirst();
		running->addRef();
	}

	UThreadState::~UThreadState() {
		threadState = null;
		if (running)
			running->release();
	}

	UThreadState *UThreadState::current() {
		assert(threadState);
		return threadState;
	}

	bool UThreadState::any() {
		Lock::L z(lock);
		return ready.any();
	}

	bool UThreadState::leave() {
		reap();

		UThreadData *prev = running;
		{
			Lock::L z(lock);
			UThreadData *next = ready.pop();
			if (!next)
				return false;
			ready.push(running);
			running = next;
		}

		// NOTE: This does not always return directly. Consider this when writing code after this statement.
		prev->switchTo(running);

		reap();
		return true;
	}

	void UThreadState::exit() {
		// Do we have something to exit to?
		UThreadData *prev = running;
		UThreadData *next = null;

		while (true) {
			{
				Lock::L z(lock);
				next = ready.pop();
			}

			if (next)
				break;

			// Wait for the signal.
			Thread::current().threadData()->wakeCond.wait();
		}

		// Now, we have something to do!
		exited.push(prev);
		running = next;
		prev->switchTo(running);

		// Should not return.
		assert(false);
	}

	void UThreadState::insert(UThreadData *data) {
		Lock::L z(lock);
		ready.push(data);
		data->addRef();
	}

	void UThreadState::reap() {
		while (UThreadData *d = exited.pop())
			d->release();
	}

	/**
	 * Machine specific code.
	 */

#ifdef X86

	static nat pageSize() {
		static nat sz = 0;
		if (sz == 0) {
			SYSTEM_INFO sysInfo;
			GetSystemInfo(&sysInfo);
			sz = sysInfo.dwPageSize;
		}
		return sz;
	}

	static void *allocStack(nat size) {
		nat pageSz = pageSize();
		size = roundUp(size, pageSz);
		size += pageSz; // we need a guard page.

		void *mem = VirtualAlloc(null, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

		DWORD oldProt;
		VirtualProtect(mem, pageSz - 1, PAGE_READONLY | PAGE_GUARD, &oldProt);

		return mem;
	}

	static void freeStack(void *base, nat size) {
		VirtualFree(base, 0, MEM_RELEASE);
	}

	static void **initialEsp(void *base, nat size) {
		byte *r = (byte *)base;
		return (void **)(r + size);
	}

	DoCallFn chooseCall(const TypeInfo &r) {
		if (r.plain() && r.kind == TypeInfo::user)
			return &doUserCall;
		else if (r.kind == TypeInfo::floatNr && r.size == sizeof(float))
			return &doFloatCall;
		else if (r.kind == TypeInfo::floatNr && r.size == sizeof(double))
			return &doDoubleCall;
		else if (r.size <= 4)
			return &doCall4;
		else if (r.size <= 8)
			return &doCall8;
		else
			return &doUserCall;
	}

	void doUserCall(UThread::Params *s, void *params) {
		const void *toCall = s->toCall;
		const void *resultFn = s->onDone;
		void *data = s->data;
		nat rSize = s->result.size;

		__asm {
			// Prepare the stack.
			sub esp, rSize;
			mov eax, esp;

			mov edi, esp;
			mov esp, params;

			// Call and restore the stack.
			push eax;
			call toCall;
			mov esp, edi;

			// Now the result is right on top of the stack!
			push data;
			call resultFn;
			add esp, 4;
			add esp, rSize;
		}
	}

	void doDoubleCall(UThread::Params *s, void *params) {
		const void *toCall = s->toCall;
		const void *resultFn = s->onDone;
		void *data = s->data;

		__asm {
			// Prepare the stack.
			mov edi, esp;
			mov esp, params;

			// Call and restore the stack.
			call toCall;
			mov esp, edi;

			// Call the 'we're done' function.
			push eax;
			push eax;
			fstp QWORD PTR [esp];
			push data;
			call resultFn;
			add esp, 12;
		}
	}

	void doFloatCall(UThread::Params *s, void *params) {
		const void *toCall = s->toCall;
		const void *resultFn = s->onDone;
		void *data = s->data;

		__asm {
			// Prepare the stack.
			mov edi, esp;
			mov esp, params;

			// Call and restore the stack.
			call toCall;
			mov esp, edi;

			// Call the 'we're done' function.
			push eax;
			fstp DWORD PTR [esp];
			push data;
			call resultFn;
			add esp, 8;
		}
	}

	void doCall4(UThread::Params *s, void *params) {
		const void *toCall = s->toCall;
		const void *resultFn = s->onDone;
		void *data = s->data;

		__asm {
			// Prepare the stack.
			mov edi, esp;
			mov esp, params;

			// Call and restore the stack.
			call toCall;
			mov esp, edi;

			// Call the 'we're done' function.
			push eax;
			push data;
			call resultFn;
			add esp, 8;
		}
	}

	void doCall8(UThread::Params *s, void *params) {
		const void *toCall = s->toCall;
		const void *resultFn = s->onDone;
		void *data = s->data;

		__asm {
			// Prepare the stack.
			mov edi, esp;
			mov esp, params;

			// Call and restore the stack.
			call toCall;
			mov esp, edi;

			// Call the 'we're done' function.
			push edx;
			push eax;
			push data;
			call resultFn;
			add esp, 12;
		}
	}

	void UThreadData::push(void *v) {
		*--esp = v;
	}

	void UThreadData::push(uintptr_t v) {
		*--esp = (void *)v;
	}

	void UThreadData::push(intptr_t v) {
		*--esp = (void *)v;
	}

	void *UThreadData::alloc(size_t s) {
		s = roundUp(s, sizeof(void *));
		esp -= s / 4;
		return esp;
	}

	void UThreadData::pushParams(const void *returnTo, void *param) {
		push(param);
		push((void *)returnTo);
	}

	void UThreadData::pushParams(const void *returnTo, const FnCall &params) {
		nat s = params.paramsSize();
		assert(s % 4 == 0);
		esp -= s / 4;
		params.copyParams(esp);
		push((void *)returnTo);
	}

	void *UThreadData::pushParams(const FnCall &params, nat minSpace) {
		minSpace += 10 * 4; // Space for the context and some return addresses.
		// Extra space for the function. VS fills about 200 bytes of the stack
		// with random data in the function prolog, so we want to have a good margin here.
		minSpace += 200 * 4;

		nat s = params.paramsSize();
		assert(s % 4 == 0);
		void **to = esp - (s + minSpace) / 4;
		params.copyParams(to);
		return to;
	}

	void UThreadData::pushContext(const void *fn) {
		push((void *)fn); // return to
		push(0); // ebp
		push(0); // ebx
		push(0); // esi
		push(0); // edi
		push(-1); // seh (end of list is -1)
		push(initialEsp(stackBase, stackSize)); // stack base
		push(stackBase); // stack limit
	}


#pragma warning (disable: 4733)
	// Note: we need to keep fs:[4] and fs:[8] updated (stack begin and end) updated
	// for exceptions to work in our threads. These could be saved through the 'stack'
	// member in the UThread itself, but this is much easier and only uses 8 bytes on the
	// stack anyway.
	NAKED void doSwitch(void *newEsp, void ***oldEsp) {
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

#endif

}
