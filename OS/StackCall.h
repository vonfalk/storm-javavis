#pragma once
#include "Stack.h"
#include "UThread.h"
#include "FnCall.h"

namespace os {

	// Internal function written in ASM that does the heavy lifting.
	extern "C" void doStackCall(Stack *current, Stack *callOn, const void *fn, const FnCallRaw *fnCall, bool member, void *result);

	// Internal helper of common code for both functions.
	inline void stackCallI(Stack *callOn, const void *fn, const FnCallRaw *fnCall, bool member, void *result) {
		dbg_assert(OFFSET_OF(Stack, desc) == sizeof(void *)*2, L"Invalid layout of the Stack object.");
		dbg_assert(OFFSET_OF(Stack, detourTo) == sizeof(void *)*4, L"Invalid layout of the Stack object.");

		callOn->clear();

		UThreadData *data = UThread::current().threadData();
		try {
			doStackCall(&data->stack, callOn, fn, fnCall, member, result);
		} catch (...) {
			// Unlink the stack. Otherwise, it will still be scanned at a later point. This is
			// likely slightly too late, but it is the best we can do right now (stack has probably
			// already been unwound from "callOn", which means that there is a window where the GC
			// will make wrong assumptions). This case is handled in the GC scanning code.
			atomicWrite(data->stack.detourTo, (Stack *)null);
			throw;
		}
	}

	// Run a function on a particular stack. Like a normal function call, but switches the stack
	// before the call. This is a bit similar to a detour, with the difference that the stack we're
	// using is not supposed to be used for other purposes. Due to this, this operation is slightly
	// cheaper than using the regular detour mechanisms.
	template <class Result, int params>
	Result stackCall(Stack &callOn, const void *fn, const FnCall<Result, params> &fnCall, bool member) {
		dbg_assert(OFFSET_OF(Stack, desc) == sizeof(void *)*2, L"Invalid layout of the Stack object.");
		dbg_assert(OFFSET_OF(Stack, detourTo) == sizeof(void *)*4, L"Invalid layout of the Stack object.");

		byte result[sizeof(Result)];
		stackCallI(&callOn, fn, &fnCall, member, result);
		Result *resPtr = (Result *)(void *)result;
		Result tmp = *resPtr;
		resPtr->~Result();
		return tmp;
	}

	template <int params>
	void stackCall(Stack &callOn, const void *fn, const FnCall<void, params> &fnCall, bool member) {
		stackCallI(&callOn, fn, &fnCall, member, null);
	}

}
