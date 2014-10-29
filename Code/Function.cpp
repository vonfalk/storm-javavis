#include "stdafx.h"
#include "Function.h"
#include "Utils/Math.h"


namespace code {

#ifdef X86
	nat FnCall::paramsSize() {
		nat size = 0;
		for (nat i = 0; i < params.size(); i++) {
			nat s = params[i].size;
			size += roundUp(s, sizeof(void *));
		}
		return size;
	}

	void FnCall::copyParams(void *to) {
		byte *at = (byte *)to;
		for (nat i = 0; i < params.size(); i++) {
			Param &p = params[i];
			nat s = roundUp(p.size, sizeof(void *));
			(*p.copy)(p.value, at);
			at += s;
		}
	}

	void FnCall::destroyParams(void *to) {
		// Note: this is not needed on X86 for Visual Studio, as the called function
		// destroys the values passed (don't ask me how it works with variable argument lists).
		// This implementation is provided as a reference implementation.
		byte *at = (byte *)to;
		for (nat i = 0; i < params.size(); i++) {
			Param &p = params[i];
			nat s = roundUp(p.size, sizeof(void *));
			(*p.destroy)(at);
			at += s;
		}
	}

	void FnCall::doCall4(void *result, void *fn) {
		nat sz = paramsSize();
		void *stackTop;
		__asm {
			sub esp, sz;
			mov stackTop, esp;
		}

		copyParams(stackTop);

		__asm {
			call fn;
			mov ebx, result;
			mov [ebx], eax;
			add esp, sz;
		}
	}

	void FnCall::doCall8(void *result, void *fn) {
		nat sz = paramsSize();
		void *stackTop;
		__asm {
			sub esp, sz;
			mov stackTop, esp;
		}

		copyParams(stackTop);

		__asm {
			call fn;
			mov ebx, result;
			mov [ebx], eax;
			mov [ebx+4], edx;
			add esp, sz;
		}
	}

	void FnCall::doCallLarge(void *result, nat retSz, void *fn) {
		nat sz = paramsSize();
		void *stackTop;
		__asm {
			sub esp, sz;
			mov stackTop, esp;
		}

		copyParams(stackTop);

		__asm {
			push result;
			call fn;
			pop result;
			add esp, sz;
		}
	}

	void FnCall::doCall(void *result, nat resultSize, void *fn) {
		resultSize = roundUp(resultSize, sizeof(void *));
		if (resultSize <= 4) {
			doCall4(result, fn);
		} else if (resultSize <= 8) {
			doCall8(result, fn);
		} else {
			doCallLarge(result, resultSize, fn);
		}
	}

	void FnCall::doUserCall(void *result, nat resultSize, void *fn) {
		doCallLarge(result, resultSize, fn);
	}

	void FnCall::callVoid(void *fn) {
		nat dummy;
		doCall4(&dummy, fn);
	}

	float FnCall::doFloatCall(void *fn) {
		return (float)doDoubleCall(fn);
	}

	double FnCall::doDoubleCall(void *fn) {
		nat sz = paramsSize();
		void *stackTop;
		__asm {
			sub esp, sz;
			mov stackTop, esp;
		}

		copyParams(stackTop);

		__asm {
			call fn;
			add esp, sz;
		}

		// we're fine! Output is already stored on the floating-point stack.
	}

#endif

}
