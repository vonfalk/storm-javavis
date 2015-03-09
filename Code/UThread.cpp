#include "stdafx.h"
#include "UThread.h"
#include "Thread.h"
#include "FnCall.h"
#include "Utils/Math.h"
#include "Utils/Lock.h"


namespace code {

	/**
	 * Forward declare machine specific functions at the bottom.
	 */

	// Stack size. (we need about 30k on Windows to do cout).
	// 40k is too small to run the compiler well.
    // TODO: Make compiler UThreads larger somehow? Maybe smaller stacks are enough in debug builds...
	static nat stackSize = 100 * 1024;

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

	UThread UThread::current() {
		return UThread(UThreadState::current()->runningThread());
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

	// Parameters to the spawn-call.
	struct Params {
		// Function to call.
		const void *fn;

		// The future to post the result to.
		FutureBase *future;

		// The type to be passed to the future object.
		BasicTypeInfo resultType;
	};

	// 'actuals' is the location on our stack of the parameters to use when calling the function in 'params'.
	// We disable the initialization check since it clears massive amounts of unused stack space.
#pragma runtime_checks("", off)
	static void spawnFuture(Params *params, void *actuals) {
		// Note: Do not call anything strange before we call p->toCall, otherwise
		// we may overwrite parameters already on the stack!

		try {
			// Call the function and place the result in the future.
			call(params->fn, actuals, params->future->target, params->resultType);
			// Notify success.
			params->future->posted();
		} catch (...) {
			// Post the error.
			params->future->error();
		}

		// Terminate ourselves.
		exitUThread();
	}
#pragma runtime_checks("", restore)

	UThread UThread::spawn(const Fn<void, void> &fn, const Thread *on) {
		UThreadData *t = UThreadData::create();

		t->pushParams(null, new Fn<void, void>(fn));
		t->pushContext(&spawnFn);

		insert(t, on);
		return UThread(t);
	}

	UThread UThread::spawn(const void *fn, const FnParams &params, const Thread *on) {
		UThreadData *t = UThreadData::create();

		t->pushParams(&exitUThread, params);
		t->pushContext(fn);

		insert(t, on);
		return UThread(t);
	}

	UThread UThread::spawn(const void *fn, const FnParams &params,
						FutureBase &result, const BasicTypeInfo &resultType,
						const Thread *on, UThreadData *t) {
		if (t == null)
			t = UThreadData::create();

		// Copy parameters to the stack of the new thread.
		Params *p = (Params *)t->alloc(sizeof(Params));
		p->fn = fn;
		p->future = &result;
		p->resultType = resultType;

		// Copy parameters a bit over the top of the stack. Some temporary stack space between is
		// inserted by the 'pushParams' function, to suit the current platform.
		void *paramsPos = t->pushParams(params, 0);

		// Set up the call to 'spawnParams'.
		t->pushParams(null, p, paramsPos);
		t->pushContext(&spawnFuture);

		// Done!
		insert(t, on);
		return UThread(t);
	}

	UThreadData *UThread::spawnLater() {
		return UThreadData::create();
	}

	void UThread::abortSpawn(UThread *data) {
		delete data;
	}


	/**
	 * UThread data.
	 */

	UThreadData::UThreadData() : references(0), next(null), owner(null), stackBase(null), stackSize(null), esp(null) {}

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

	UThreadState::UThreadState(ThreadData *owner) : owner(owner) {
		threadState = this;

		running = UThreadData::createFirst();
		running->owner = this;
		running->addRef();

		aliveCount = 1;
	}

	UThreadState::~UThreadState() {
		threadState = null;
		if (running) {
			running->release();
			atomicDecrement(aliveCount);
		}

		assert(aliveCount == 0, L"An OS thread tried to terminate before all its UThreads terminated.");
	}

	UThreadState *UThreadState::current() {
		assert(threadState);
		return threadState;
	}

	bool UThreadState::any() {
		return atomicCAS(aliveCount, 1, 1) != 1;
	}

	bool UThreadState::leave() {
		reap();

		UThreadData *prev = running;
		{
			::Lock::L z(lock);
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
				::Lock::L z(lock);
				next = ready.pop();
			}

			if (next)
				break;

			// Wait for the signal.
			owner->wakeCond.wait();
		}

		// Now, we have something to do!
		atomicDecrement(aliveCount);
		exited.push(prev);
		running = next;
		prev->switchTo(running);

		// Should not return.
		assert(false);
	}

	void UThreadState::insert(UThreadData *data) {
		data->owner = this;
		atomicIncrement(aliveCount);

		::Lock::L z(lock);
		ready.push(data);
		data->addRef();
	}

	void UThreadState::wait() {
		UThreadData *prev = running;
		UThreadData *next = null;

		while (true) {
			{
				::Lock::L z(lock);
				next = ready.pop();
			}

			if (next == prev) {
				// May happen if we are the only UThread running.
				break;
			} else if (next) {
				running = next;
				prev->switchTo(running);
				break;
			}

			// Nothing to do in the meantime...
			owner->wakeCond.wait();
		}

		reap();
	}

	void UThreadState::wake(UThreadData *data) {
		::Lock::L z(lock);
		ready.push(data);
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

	void *UThread::spawnParamMem(UThreadData *data) {
		// The stack grows from the bottom, so we can reuse the top for parameters.
		return data->stackBase;
	}

	static void *allocStack(nat size) {
		nat pageSz = pageSize();
		size = roundUp(size, pageSz);
		size += pageSz; // we need a guard page.

		byte *mem = (byte *)VirtualAlloc(null, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

		DWORD oldProt;
		VirtualProtect(mem, 1, PAGE_READONLY | PAGE_GUARD, &oldProt);

		// Do not show the guard page to other parts...
		return mem + pageSz;
	}

	static void freeStack(void *base, nat size) {
		byte *mem = (byte *)base;
		mem -= pageSize();
		VirtualFree(mem, 0, MEM_RELEASE);
	}

	static void **initialEsp(void *base, nat size) {
		byte *r = (byte *)base;
		return (void **)(r + size);
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

	void UThreadData::pushParams(const void *returnTo, void *param1, void *param2) {
		push(param2);
		push(param1);
		push((void *)returnTo);
	}

	void UThreadData::pushParams(const void *returnTo, const FnParams &params) {
		nat s = params.totalSize();
		assert(s % 4 == 0);
		esp -= s / 4;
		params.copy(esp);
		push((void *)returnTo);
	}

	void *UThreadData::pushParams(const FnParams &params, nat minSpace) {
		minSpace += 30 * 4; // Space for the context and some return addresses.
		// Extra space for the function. VS fills about 200 machine words of the stack
		// with random data in the function prolog, so we want to have a good margin here.
		// In release builds, we can probably reduce this a lot.
#ifdef DEBUG
		// In debug builds, the visual studio compiler allocates more stack than needed.
		// I have removed some waste (including filling with 0xCC) by disabling runtime
		// checks on relevant functions.
		minSpace += 120 * 4;
#endif

		nat s = params.totalSize();
		assert(s % 4 == 0);
		void **to = esp - (s + minSpace) / 4;
		params.copy(to);
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
