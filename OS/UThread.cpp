#include "stdafx.h"
#include "UThread.h"
#include "Thread.h"
#include "FnCall.h"
#include "Shared.h"
#include "Sync.h"
#include "Utils/Bitwise.h"
#include "Utils/Lock.h"
#include "UThreadX64.h"
#include <limits>

#ifdef POSIX
#include <sys/mman.h>
#endif

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
	void doSwitch(StackDesc **newEsp, StackDesc **oldEsp);

	// Switch back to the previously running UThread, allocating an additional return address on the
	// stack and remembering the location of that stack so that it can be altered later.
	// Returns the function that shall be called, since there may be variants of the function.
	static const void *endDetourFn(bool member);

	// Allocate a new stack.
	static void *allocStack(nat size);

	// Free a previously allocated stack.
	static void freeStack(void *base, nat size);

	// Compute the initial stack description for this architecture.
	static StackDesc *initialDesc(void *base, nat size);

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

	static ThreadData *threadData(const Thread *thread) {
		if (thread && *thread != Thread::invalid)
			return thread->threadData();
		return Thread::current().threadData();
	}

	// Insert on a specific thread. Makes sure to keep 'data' alive until the function has returned at least.
	UThread UThread::insert(UThreadData *data, ThreadData *on) {
		// We need to take our reference here, from the moment we call 'insert', 'data' may be deleted otherwise!
		UThread result(data);

		on->uState.insert(data);
		on->reportWake();

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

	UThread UThread::spawn(const util::Fn<void, void> &fn, const Thread *on) {
		ThreadData *thread = os::threadData(on);
		UThreadData *t = UThreadData::create(&thread->uState);

		t->pushContext(address(&spawnFn), new util::Fn<void, void>(fn));

		return insert(t, thread);
	}

	struct SpawnParams {
		bool memberFn;
		void *first;
		void **params;
		CallThunk thunk;

		// Only valid when using futures.
		void *target;
		FutureBase *future;
	};

	static void spawnCall(SpawnParams *params) {
		try {
			(*params->thunk)(endDetourFn(params->memberFn), params->memberFn, params->params, params->first, null);
		} catch (...) {
			onUncaughtException();
		}

		// Terminate.
		exitUThread();
	}

	static void spawnCallFuture(SpawnParams *params) {
		// We need to save a local copy of 'future' since 'params' reside on the stack of the
		// caller, which will continue as soon as we try to call the function.
		FutureBase *future = params->future;

		try {
			(*params->thunk)(endDetourFn(params->memberFn), params->memberFn, params->params, params->first, params->target);
			future->posted();
		} catch (...) {
			future->error();
		}

		// Terminate.
		exitUThread();
	}

	typedef void (*SpawnFn)(SpawnParams *params);

	static UThreadData *spawnHelper(SpawnFn spawn, const void *fn, ThreadData *thread, SpawnParams *params) {
		// Create a new UThread on the proper thread.
		UThreadData *t = UThreadData::create(&thread->uState);

		// Set up the thread for calling 'spawnCall'.
		t->pushContext((const void *)spawn, params);

		// Call the newly created UThread on this thread, and make sure to return directly here later.
		UThreadState *current = UThreadState::current();
		void *returnAddr = current->startDetour(t);

		// Make sure we return to the function we were supposed to call.
		*(const void **)returnAddr = fn;

		// Done!
		return t;
	}

	UThread UThread::spawnRaw(const void *fn, bool memberFn, void *first, const FnCallRaw &call, const Thread *on) {
		ThreadData *thread = os::threadData(on);

		SpawnParams params = {
			memberFn,
			first,
			call.params(),
			call.thunk,
			null,
			null
		};
		UThreadData *t = spawnHelper(&spawnCall, fn, thread, &params);
		return insert(t, thread);
	}

	UThread UThread::spawnRaw(const void *fn, bool memberFn, void *first, const FnCallRaw &call, FutureBase &result,
							void *target, const Thread *on) {
		ThreadData *thread = os::threadData(on);

		SpawnParams params = {
			memberFn,
			first,
			call.params(),
			call.thunk,
			target,
			&result
		};
		UThreadData *t = spawnHelper(&spawnCallFuture, fn, thread, &params);
		return insert(t, thread);
	}


	/**
	 * UThread data and UThreadStack.
	 */

	UThreadStack::UThreadStack() : desc(null), stackLimit(null) {}

	UThreadData::UThreadData(UThreadState *state) :
		references(0), next(null), owner(null), stackBase(null), stackSize(0) {

		// Notify the GC that we exist and may contain interesting data.
		state->newStack(this);
	}

	UThreadData *UThreadData::createFirst(UThreadState *thread, void *base) {
		// For the thread where the stack was allocated by the OS, we do not need to care.
		UThreadData *t = new UThreadData(thread);
		t->stack.stackLimit = base;
		return t;
	}

	UThreadData *UThreadData::create(UThreadState *thread) {
		// 'allocStack' may throw an exception, so make sure it succeeds before proceeding any further.
		void *stack = allocStack(os::stackSize);

		UThreadData *t = new UThreadData(thread);
		t->stackSize = os::stackSize;
		t->stackBase = stack;
		t->stack.desc = initialDesc(t->stackBase, t->stackSize);
		t->stack.stackLimit = t->stack.desc->high;
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

	UThreadState::UThreadState(ThreadData *owner, void *stackBase) : owner(owner) {
		currentUThreadState(this);

		running = UThreadData::createFirst(this, stackBase);
		running->owner = this;
		running->addRef();

		aliveCount = 1;
		detourOrigin = null;
	}

	UThreadState::~UThreadState() {
		currentUThreadState(null);
		if (running) {
			stacks.erase(&running->stack);
			running->release();
			atomicDecrement(aliveCount);
		}

		assert(detourOrigin == null, L"A detour was not properly terminated before terminating the thread.");
		assert(aliveCount == 0, L"An OS thread tried to terminate before all its UThreads terminated.");
		assert(stacks.empty(), L"Consistency issue. aliveCount says zero, but we still have stacks we can scan!");
	}

	void UThreadState::newStack(UThreadData *v) {
		util::Lock::L z(lock);
		stacks.insert(&v->stack);
	}

	UThreadState *UThreadState::current() {
		UThreadState *s = currentUThreadState();
		if (!s) {
			Thread::current();
			s = currentUThreadState();
			assert(s);
		}
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

		// Any IO messages for this thread?
		owner->checkIo();

		// NOTE: This does not always return directly. Consider this when writing code after this statement.
		prev->switchTo(running);

		reap();
		return true;
	}

	void UThreadState::sleep(nat ms) {
		struct D : public SleepData {
			D(int64 until) : SleepData(until), sema(0) {}
			os::Sema sema;
			virtual void signal() {
				sema.up();
			}
		};

		D done(timestamp() + msInTimestamp(ms));
		sleeping.push(&done);
		done.sema.down();
	}

	bool UThreadState::nextWake(nat &time) {
		SleepData *first = sleeping.peek();
		if (!first)
			return false;

		time = remainingMs(first->until);
		return true;
	}

	void UThreadState::wakeThreads() {
		wakeThreads(timestamp());
	}

	void UThreadState::wakeThreads(int64 time) {
		while (sleeping.any()) {
			SleepData *first = sleeping.peek();
			if (time >= first->until) {
				first->signal();
				sleeping.pop();
			} else {
				break;
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

		wakeThreads(timestamp());
		owner->checkIo();
	}

	void *UThreadState::startDetour(UThreadData *to) {
		assert(to->owner == null, L"The UThread is already associated with a thread, can not use it for detour.");
		assert(detourOrigin == null, L"Can not start multiple detours!");
		to->owner = this;

		detourOrigin = running;
		running = to;
		detourOrigin->switchTo(running);
		return detourResult;
	}

	void UThreadState::endDetour(void *result) {
		assert(detourOrigin, L"No active detour.");
		detourResult = result;
		running->owner = null;

		UThreadData *prev = running;
		running = detourOrigin;
		detourOrigin = null;
		prev->switchTo(running);
	}

	/**
	 * Machine specific code.
	 */

#if defined(X86)

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

#ifdef DEBUG
	static nat stacks = 0;
#endif

	static void *allocStack(nat size) {
		nat pageSz = pageSize();
		size = roundUp(size, pageSz);
		size += pageSz; // We want a guard page.

		byte *mem = (byte *)VirtualAlloc(null, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		if (mem == null) {
			// TODO: What to do in this case?
#ifdef DEBUG
			PLN(L"Out of memory when spawning a thread. Threads alive: " << stacks);
#endif
			throw ThreadError(L"Out of memory when spawning a thread.");
		}

#ifdef DEBUG
		atomicIncrement(stacks);
#endif

		DWORD oldProt;
		VirtualProtect(mem, 1, PAGE_READONLY | PAGE_GUARD, &oldProt);

		// Do not show the guard page to other parts...
		return mem + pageSz;
	}

	static void freeStack(void *base, nat size) {
		byte *mem = (byte *)base;
		mem -= pageSize();
		VirtualFree(mem, 0, MEM_RELEASE);

#ifdef DEBUG
		atomicDecrement(stacks);
#endif
	}

	static StackDesc *initialDesc(void *base, nat size) {
		// Put the initial stack description in the 'top' of the stack.
		// Update it whenever we call 'pushContext'.
		byte *r = (byte *)base;
		StackDesc *desc = (StackDesc *)base;

		desc->low = r + size;
		desc->high = r + size;

		return desc;
	}

	static void *&stackPtr(UThreadStack &stack) {
		return stack.desc->low;
	}

	void UThreadData::push(void *v) {
		void **esp = (void **)stackPtr(stack);
		*--esp = v;
		stackPtr(stack) = esp;
	}

	void UThreadData::push(uintptr_t v) {
		void **esp = (void **)stackPtr(stack);
		*--esp = (void *)v;
		stackPtr(stack) = esp;
	}

	void UThreadData::push(intptr_t v) {
		void **esp = (void **)stackPtr(stack);
		*--esp = (void *)v;
		stackPtr(stack) = esp;
	}

	void UThreadData::pushContext(const void *fn) {
		push((void *)fn); // return to
		push(0); // ebp
		push(0); // ebx
		push(0); // esi
		push(0); // edi
		push(-1); // seh (end of list is -1)
		push(stack.desc->high);
		push(stackBase); // stack limit
		push(stack.desc->low); // current stack pointer (approximate, this is enough)

		// Set the 'desc' of 'stack' to the actual stack pointer, so that we can run code on this stack.
		stack.desc = (StackDesc *)(stack.desc->low);
	}

	void UThreadData::pushContext(const void *fn, void *param) {
		push(param);
		push(null); // return to
		pushContext(fn);
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
			mov edx, newDesc; // Trash edx so that the compiler do not accidentally assume it is untouched.

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

			// Report the state for the old thread. This makes it possible for the GC to scan that properly.
			mov [eax], esp;

			// Switch to the new stack.
			mov esp, [ecx];

			// Restore state.
			add esp, 4; // skip 'high'.
			pop dword ptr fs:[8];
			pop dword ptr fs:[4];
			pop dword ptr fs:[0];
			pop edi;
			pop esi;
			pop ebx;

			// Clear the new thread's description. This means that the GC will start scanning the
			// stack based on esp rather than what is in the StackDesc.
			mov DWORD PTR [ecx], 0;

			// Epilog. Bonus: restores ebp!
			pop ebp;
			ret;
		}
	}

	void doEndDetour2() {
		// Figure out the return address from the previous function.
		void *retAddr = null;
		__asm {
			lea eax, [ebp+4];
			mov retAddr, eax;
		}

		// Then call 'endDetour' of the current UThread.
		UThreadState::current()->endDetour(retAddr);
	}

	NAKED void doEndDetour() {
		// 'allocate' an extra return address by simply calling yet another function. This return
		// address can be modified to simulate a direct jump to another function with the same
		// parameters that were passed to this function.
		__asm {
			call doEndDetour2;
			ret; // probably not reached, but better safe than sorry!
		}
	}

	static const void *endDetourFn(bool member) {
		return address(&doEndDetour);
	}

#elif defined(GCC) && defined(X64)

	static int64 timestamp() {
		struct timespec time = {0, 0};
		clock_gettime(CLOCK_MONOTONIC, &time);

		int64 r = time.tv_sec;
		r *= 1000 * 1000;
		r += time.tv_nsec / 1000;
		return r;
	}

	static int64 msInTimestamp(nat ms) {
		return int64(ms * 1000);
	}

	static nat remainingMs(int64 target) {
		int64 now = timestamp();
		if (target <= now)
			return 0;

		int64 remaining = target - now;
		return nat(remaining / 1000);
	}

	static nat pageSize() {
		static nat sz = 0;
		if (sz == 0) {
			int s = getpagesize();
			assert(s > 0, L"Failed to acquire the page size for your system!");
			sz = (nat)s;
		}
		return sz;
	}

#ifdef DEBUG
	static nat stacks = 0;
#endif

	static void *allocStack(nat size) {
		nat pageSz = pageSize();
		size = roundUp(size, pageSz);
		size += pageSz; // We want a guard page.

		byte *mem = (byte *)mmap(null, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (mem == null) {
			// TODO: What to do in this case?
#ifdef DEBUG
			PLN(L"Out of memory when spawning a thread. Threads alive: " << stacks);
#endif
			throw ThreadError(L"Out of memory when spawning a thread.");
		}

#ifdef DEBUG
		atomicIncrement(stacks);
#endif

		mprotect(mem, 1, PROT_NONE); // no special guard-page it seems...

		// Do not show the guard page to other parts...
		return mem + pageSz;
	}

	static void freeStack(void *base, nat size) {
		byte *mem = (byte *)base;
		nat pageSz = pageSize();
		mem -= pageSz;
		munmap(mem, size + pageSz);

#ifdef DEBUG
		atomicDecrement(stacks);
#endif
	}

	static StackDesc *initialDesc(void *base, nat size) {
		// Put the initial stack description in the 'top' of the stack.
		// Update it whenever we call 'pushContext'.
		byte *r = (byte *)base;
		StackDesc *desc = (StackDesc *)base;

		desc->low = r + size;
		desc->high = r + size;

		return desc;
	}

	static void *&stackPtr(UThreadStack &stack) {
		return stack.desc->low;
	}

	void UThreadData::push(void *v) {
		void **esp = (void **)stackPtr(stack);
		*--esp = v;
		stackPtr(stack) = esp;
	}

	void UThreadData::push(uintptr_t v) {
		void **esp = (void **)stackPtr(stack);
		*--esp = (void *)v;
		stackPtr(stack) = esp;
	}

	void UThreadData::push(intptr_t v) {
		void **esp = (void **)stackPtr(stack);
		*--esp = (void *)v;
		stackPtr(stack) = esp;
	}

	void UThreadData::pushContext(const void *fn) {
		pushContext(fn, null);
	}

	static void errorFn() {
		PLN(L"UThread boot function returned. This is an implementation bug!");
		std::terminate();
	}

	void UThreadData::pushContext(const void *fn, void *param) {
		// Previous function's return to. Needed to align the stack properly.
		push((void *)errorFn);

		push((void *)fn); // return to
		push((void *)0); // rbp
		push(param); // rdi
		push((void *)0); // rbx
		push((void *)0); // r12
		push((void *)0); // r13
		push((void *)0); // r14
		push((void *)0); // r15

		push(stack.desc->high);
		push(stack.desc->dummy);
		push(stack.desc->low);

		// Set the 'desc' of 'stack' to the actual stack pointer, so that we can run code on this stack.
		stack.desc = (StackDesc *)(stack.desc->low);
	}

	// Called from UThreadX64.S
	extern "C" void doEndDetour2(void **result) {
		UThreadState::current()->endDetour(result);
	}

	extern "C" void doEndDetour();
	extern "C" void doEndDetourMember();

	static const void *endDetourFn(bool member) {
		return member
			? address(doEndDetourMember)
			: address(doEndDetour);
	}

#endif

}
