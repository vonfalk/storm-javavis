#include "stdafx.h"
#include "FnCall.h"

namespace code {

#ifdef X86

	// Disable run time checks here, since we are called from UThread and need to preserve
	// the stack in order to not destroy parameters stored on higher parts of the stack.
#pragma runtime_checks("", off)

	// Stack overhead by the 'CallFn' functions.
	// call, push in the functions and 3 parameters. (+ some spare).
	const nat doCallStack = 8 * sizeof(void *);

	/**
	 * Do various calls. These are declared as NAKED as we need to know an
	 * upper bound on their stack consumption.
	 */


	static NAKED void doCallLarge(const void *fn, void *params, void *result) {
		__asm {
			// Set up the stack.
			push ebp;
			mov ebp, esp;
			mov esp, params;
			push result;
			call fn;

			// Resore the stack.
			mov esp, ebp;
			pop ebp;
			ret;
		}
	}

	static NAKED void doCall0(const void *fn, void *params, void *result) {
		__asm {
			// Set up the stack.
			push ebp;
			mov ebp, esp;
			mov esp, params;
			call fn;

			// Restore the stack (no result).
			mov esp, ebp;
			pop ebp;
			ret;
		}
	}

	static NAKED void doCall4(const void *fn, void *params, void *result) {
		__asm {
			// Set up the stack.
			push ebp;
			mov ebp, esp;
			mov esp, params;
			call fn;

			// Restore the stack and store result.
			mov ecx, result;
			mov [ecx], eax;

			mov esp, ebp;
			pop ebp;
			ret;
		}
	}

	static NAKED void doCall8(const void *fn, void *params, void *result) {
		__asm {
			// Set up the stack.
			push ebp;
			mov ebp, esp;
			mov esp, params;
			call fn;

			// Restore the stack and store result.
			mov ecx, result;
			mov [ecx], eax;
			mov [ecx+4], edx;

			mov esp, ebp;
			pop ebp;
			ret;
		}
	}

	static NAKED void doCallFloat(const void *fn, void *params, void *result) {
		__asm {
			// Set up the stack.
			push ebp;
			mov ebp, esp;
			mov esp, params;
			call fn;

			// Restore the stack and store result.
			mov ecx, result;
			fstp DWORD PTR [ecx];

			mov esp, ebp;
			pop ebp;
			ret;
		}
	}

	static NAKED void doCallDouble(const void *fn, void *params, void *result) {
		__asm {
			// Set up the stack.
			push ebp;
			mov ebp, esp;

			mov esp, params;
			call fn;

			// Restore the stack and store result.
			mov ecx, result;
			fstp QWORD PTR [ecx];

			mov esp, ebp;
			pop ebp;
			ret;
		}
	}

	typedef void (*CallFn)(const void *, void *, void *);

	static CallFn chooseCall(const BasicTypeInfo &info) {
		switch (info.kind) {
		case BasicTypeInfo::user:
			if (info.plain())
				return &doCallLarge;
			else
				// We're on X86, references and pointers are 4 bytes.
				return &doCall4;
		case BasicTypeInfo::floatNr:
			if (info.size == 4)
				return &doCallFloat;
			else
				return &doCallDouble;
		case BasicTypeInfo::ptr:
			return &doCall4;
		case BasicTypeInfo::nothing:
			return &doCall0;
		}

		if (info.size <= 4)
			return &doCall4;
		else if (info.size <= 8)
			return &doCall8;
		else
			return &doCallLarge;
	}

	void call(const void *fn, void *params, void *result, const BasicTypeInfo &info) {
		CallFn z = chooseCall(info);
		(*z)(fn, params, result);
	}

	void call(const void *fn, const FnParams &params, void *result, const BasicTypeInfo &info) {
		CallFn z = chooseCall(info);
		nat sz = params.totalSize();

		__asm {
			// Allocate the parameters a bit over the top of the stack.
			// We know that we will only use 'doCallStack' from the time we
			// have copied the parameters until we manage to call the function
			// itself.
			mov esi, esp;
			sub esp, doCallStack;
			sub esp, sz;
			mov edi, esp;

			// Copy parameters! (note that we run this function 'on top of' the parameter
			// so that it does not interfere with the parameters being copied).
			mov ecx, params;
			push edi;
			call FnParams::copy;

			// Restore the stack and call the function. This is where
			// we need to not use up more than 'doCallStack' bytes on the stack.
			mov esp, esi;
			push result;
			push edi;
			push fn;
			call z;
			add esp, 12;
		}
	}

#pragma runtime_checks("", restore)

#else
#error "Please implement function calls for your architecture!"
#endif

}
