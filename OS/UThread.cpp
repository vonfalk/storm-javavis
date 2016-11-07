#include "stdafx.h"
#include "UThread.h"
#include "Thread.h"
#include "FnCall.h"
#include "Shared.h"
#include "Utils/Bitwise.h"
#include "Utils/Lock.h"

namespace os {

	/**
	 * Forward declare machine specific functions at the bottom.
	 */

	typedef UThreadStack::Desc StackDesc;

	// Stack size. (we need about 30k on Windows to do cout).
	// 40k is too small to run the compiler well.
    // TODO: Make compiler UThreads larger somehow? Maybe smaller stacks are enough in release builds...
	static nat stackSize = 400 * 1024;

	// Switch the currently running threads. *oldEsp is set to the old esp.
	// This returns as another thread, which may mean that it returns to the
	// beginning of another function in the case of newly started UThreads.
	void doSwitch(StackDesc **newEsp, StackDesc **oldEsp); // can not be static for some reason...

	// Allocate a new stack.
	static void *allocStack(nat size);

	// Free a previously allocated stack.
	static void freeStack(void *base, nat size);

	// Compute the initial esp for this architecture.
	static StackDesc *initialStack(void *base, nat size);

	// Get a platform-dependent highly accurate timestamp in some unit. This should be monotonically
	// increasing.
	static int64 timestamp();

	// Convert a millisecond interval into the units of the high frequency unit.
	static int64 msInTimestamp(nat ms);

	// Check how much time remains until a given timestamp (in ms).
	static nat remainingMs(int64 timestamp);


	/**
	 * UThread.
	 */

	// Insert on a specific thread. Makes sure to keep 'data' alive until the function has returned at least.
	UThread UThread::insert(UThreadData *data, const Thread *on) {
		ThreadData *d = null;
		if (on) {
			assert(*on != Thread::invalid);
			d = on->threadData();
		} else {
			d = Thread::current().threadData();
		}

		// We need to take our reference here, from the moment we call 'insert', 'data' may be deleted otherwise!
		UThread result(data);

		d->uState.insert(data);
		d->reportWake();

		return result;
	}

	// End the current thread.
	static void exitUThread() {
		UThreadState::current()->exit();
	}

	const UThread UThread::invalid(null);

	UThread::UThread(UThreadData *data) : data(data) {
		if (data)
			data->addRef();
	}

	UThread::UThread(const UThread &o) : data(o.data) {
		if (data)
			data->addRef();
	}

	UThread &UThread::operator =(const UThread &o) {
		if (data)
			data->release();
		data = o.data;
		if (data)
			data->addRef();
		return *this;
	}

	UThread::~UThread() {
		if (data)
			data->release();
	}

	bool UThread::leave() {
		return UThreadState::current()->leave();
	}

	void UThread::sleep(nat ms) {
		UThreadState::current()->sleep(ms);
	}

	bool UThread::any() {
		return UThreadState::current()->any();
	}

	UThread UThread::current() {
		return UThread(UThreadState::current()->runningThread());
	}

	static void onUncaughtException() {
		try {
			throw;
		} catch (const Exception &e) {
			PLN("Uncaught exception from UThread: " << e);
		} catch (...) {
			PLN("Uncaught exception from UThread: <unknown>");
		}
	}

	static void spawnFn(util::Fn<void, void> *fn) {
		try {
			(*fn)();
		} catch (...) {
			onUncaughtException();
		}

		delete fn;
		exitUThread();
	}

	// Parameters to the spawn-call.
	struct Params {
		// Function to call.
		const void *fn;

		// Member function?
		bool memberFn;

		// The type to be passed to the future object.
		BasicTypeInfo resultType;
	};

	struct FutureParams : Params {
		// The future to post the result to.
		FutureBase *future;
	};

	// 'actuals' is the location on our stack of the parameters to use when calling the function in 'params'.
	// We disable the initialization check since it clears massive amounts of unused stack space.
#pragma runtime_checks("", off)
	static void spawnParams(Params *params, void *actuals) {
		// Note: Do not call anything strange before we call p->toCall, otherwise
		// we may overwrite parameters already on the stack!

		try {
			// Call the function and place the result in the future.
			call(params->fn, params->memberFn, actuals, null, params->resultType);
		} catch (...) {
			onUncaughtException();
		}

		// Terminate ourselves.
		exitUThread();
	}

	static void spawnFuture(FutureParams *params, void *actuals) {
		// Note: Do not call anything strange before we call p->toCall, otherwise
		// we may overwrite parameters already on the stack!

		try {
			// Call the function and place the result in the future.
			call(params->fn, params->memberFn, actuals, params->future->target, params->resultType);
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

	UThread UThread::spawn(const util::Fn<void, void> &fn, const Thread *on) {
		UThreadData *t = UThreadData::create();

		t->pushParams(null, new util::Fn<void, void>(fn));
		t->pushContext(&spawnFn);

		return insert(t, on);
	}

	UThread UThread::spawn(const void *fn, bool memberFn, const FnParams &params, const Thread *on, UThreadData *t) {
		if (t == null)
			t = UThreadData::create();

		// Copy parameters to the stack of the new thread.
		Params *p = (Params *)t->alloc(sizeof(Params));
		p->fn = fn;
		p->memberFn = memberFn;
		p->resultType = typeInfo<void>();

		// Copy parameters a bit over the top of the stack. Some temporary stack space between is
		// inserted by the 'pushParams' function, to suit the current platform.
		void *paramsPos = t->pushParams(params, 0);

		// Set up the call!
		t->pushParams(null, p, paramsPos);
		t->pushContext(&spawnParams);

		// Done!
		return insert(t, on);
	}

	UThread UThread::spawn(const void *fn, bool memberFn, const FnParams &params,
						FutureBase &result, const BasicTypeInfo &resultType,
						const Thread *on, UThreadData *t) {
		if (t == null)
			t = UThreadData::create();

		// Copy parameters to the stack of the new thread.
		FutureParams *p = (FutureParams *)t->alloc(sizeof(FutureParams));
		p->fn = fn;
		p->memberFn = memberFn;
		p->future = &result;
		p->resultType = resultType;

		// Copy parameters a bit over the top of the stack. Some temporary stack space between is
		// inserted by the 'pushParams' function, to suit the current platform.
		void *paramsPos = t->pushParams(params, 0);

		// Set up the call to 'spawnParams'.
		t->pushParams(null, p, paramsPos);
		t->pushContext(&spawnFuture);

		// Done!
		return insert(t, on);
	}

	UThreadData *UThread::spawnLater() {
		return UThreadData::create();
	}

	void UThread::abortSpawn(UThreadData *data) {
		if (data)
			delete data;
	}


	/**
	 * UThread data and UThreadStack.
	 */

	UThreadStack::UThreadStack() : desc(null) {}

	UThreadData::UThreadData() : references(0), next(null), owner(null), stackBase(null), stackSize(null) {}

	UThreadData *UThreadData::createFirst() {
		// For the thread where the stack was allocated by the OS, we do not need to care.
		return new UThreadData();
	}

	UThreadData *UThreadData::create() {
		UThreadData *t = new UThreadData();
		t->stackSize = os::stackSize;
		t->stackBase = allocStack(t->stackSize);
		t->stack.desc = initialStack(t->stackBase, t->stackSize);
		return t;
	}

	UThreadData::~UThreadData() {
		// Remove from the global list in UThreadState.
		if (stackBase)
			freeStack(stackBase, stackSize);
	}

	void UThreadData::switchTo(UThreadData *to) {
		doSwitch(&to->stack.desc, &stack.desc);
	}

	/**
	 * UThread state.
	 */

	UThreadState::UThreadState(ThreadData *owner) : owner(owner) {
		currentUThreadState(this);

		running = UThreadData::createFirst();
		running->owner = this;
		running->addRef();

		stacks.insert(&running->stack);
		aliveCount = 1;
	}

	UThreadState::~UThreadState() {
		currentUThreadState(null);
		if (running) {
			stacks.erase(&running->stack);
			running->release();
			atomicDecrement(aliveCount);
		}

		assert(aliveCount == 0, L"An OS thread tried to terminate before all its UThreads terminated.");
		assert(stacks.empty(), L"Consistency issue. aliveCount says zero, but we still have stacks we can scan!");
	}

	UThreadState *UThreadState::current() {
		UThreadState *s = currentUThreadState();
		assert(s); // TDOD: create 's' if it is not created at this point. It can happen, it is however unlikely.
		return s;
	}

	bool UThreadState::any() {
		return atomicRead(aliveCount) != 1;
	}

	bool UThreadState::leave() {
		reap();

		UThreadData *prev = running;
		{
			util::Lock::L z(lock);
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

	void UThreadState::sleep(nat ms) {
		int64 doneAt = timestamp() + msInTimestamp(ms);
		bool done = false;

		while (!done) {
			do {
				done = doneAt < timestamp();
			} while (leave() && !done);

			if (!done) {
				owner->waitForWork(remainingMs(doneAt));
				done = doneAt < timestamp();
			}
		}
	}

	void UThreadState::exit() {
		// Do we have something to exit to?
		UThreadData *prev = running;
		UThreadData *next = null;

		while (true) {
			{
				util::Lock::L z(lock);
				next = ready.pop();
			}

			if (next)
				break;

			// Wait for the signal.
			owner->waitForWork();
		}

		// Now, we have something else to do!
		{
			util::Lock::L z(lock);
			stacks.erase(&prev->stack);
		}
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

		util::Lock::L z(lock);
		stacks.insert(&data->stack);
		ready.push(data);
		data->addRef();
	}

	void UThreadState::wait() {
		UThreadData *prev = running;
		UThreadData *next = null;

		while (true) {
			{
				util::Lock::L z(lock);
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
			owner->waitForWork();
		}

		reap();
	}

	void UThreadState::wake(UThreadData *data) {
		util::Lock::L z(lock);
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

	static int64 timestamp() {
		LARGE_INTEGER v;
		QueryPerformanceCounter(&v);
		return v.QuadPart;
	}

	static int64 msInTimestamp(nat ms) {
		LARGE_INTEGER v;
		QueryPerformanceFrequency(&v);
		return (v.QuadPart * ms) / 1000;
	}

	static nat remainingMs(int64 target) {
		int64 now = timestamp();
		if (target <= now)
			return 0;

		LARGE_INTEGER v;
		QueryPerformanceFrequency(&v);
		return nat((1000 * (target - now)) / v.QuadPart);
	}

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

	static StackDesc *initialStack(void *base, nat size) {
		byte *r = (byte *)base;
		return (StackDesc *)(r + size);
	}

	void UThreadData::push(void *v) {
		void **esp = (void **)stack.desc;
		*--esp = v;
		stack.desc = (StackDesc *)esp;
	}

	void UThreadData::push(uintptr_t v) {
		void **esp = (void **)stack.desc;
		*--esp = (void *)v;
		stack.desc = (StackDesc *)esp;
	}

	void UThreadData::push(intptr_t v) {
		void **esp = (void **)stack.desc;
		*--esp = (void *)v;
		stack.desc = (StackDesc *)esp;
	}

	void *UThreadData::alloc(size_t s) {
		s = roundUp(s, sizeof(void *));
		void **esp = (void **)stack.desc;
		esp -= s / 4;
		stack.desc = (StackDesc *)esp;
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
		void **esp = (void **)stack.desc;
		esp -= s / 4;
		stack.desc = (StackDesc *)esp;
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
		void **esp = (void **)stack.desc;
		void **to = esp - (s + minSpace) / 4;
		stack.desc = (StackDesc *)esp;
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
		push(initialStack(stackBase, stackSize)); // stack base
		push(stackBase); // stack limit
		push(stack.desc); // current stack-pointer (approximate, this is enough).
	}


#pragma warning (disable: 4733)
	// Note: we need to keep fs:[4] and fs:[8] updated (stack begin and end) updated
	// for exceptions to work in our threads. These could be saved through the 'stack'
	// member in the UThread itself, but this is much easier and only uses 8 bytes on the
	// stack anyway.
	NAKED void doSwitch(StackDesc **newDesc, StackDesc **oldDesc) {
		__asm {
			// Prolog. Bonus: saves ebp!
			push ebp;
			mov ebp, esp;

			// Store parameters.
			mov eax, oldDesc;
			mov ecx, newDesc;

			// Store state.
			push ebx;
			push esi;
			push edi;
			push dword ptr fs:[0];
			push dword ptr fs:[4];
			push dword ptr fs:[8];

			// Current stack pointer, overlaps with 'high' in StackDesc.
			lea ebx, [esp-4];
			push ebx;

			// Report the state for the old thread. This makes it possible for the gc to scan that properly.
			mov [eax], esp;

			// Switch to the new stack. At this point, the GC will ignore the current thread, as it
			// has seen it 'disabled' earlier.
			mov esp, [ecx];

			// Restore state.
			add esp, 4; // skip 'high'.
			pop dword ptr fs:[8];
			pop dword ptr fs:[4];
			pop dword ptr fs:[0];
			pop edi;
			pop esi;
			pop ebx;

			// Clear the new thread's description. This means that the gc will start scanning the
			// stack based on esp rather than what is in the StackDesc.
			mov DWORD PTR [ecx], 0;

			// Epilog. Bonus: restores ebp!
			pop ebp;
			ret;
		}
	}

#endif

}
