#include "stdafx.h"
#include "Function.h"
#include "Utils/Math.h"


namespace code {

#ifdef X86
	nat FnCall::paramsSize() const {
		nat size = 0;
		for (nat i = 0; i < params.size(); i++) {
			nat s = params[i].size;
			size += roundUp(s, sizeof(void *));
		}
		return size;
	}

	void FnCall::copyParams(void *to) const {
		byte *at = (byte *)to;
		for (nat i = 0; i < params.size(); i++) {
			const Param &p = params[i];
			nat s = roundUp(p.size, sizeof(void *));
			(*p.copy)(p.value, at);
			at += s;
		}
	}

	void FnCall::destroyParams(void *to) const {
		// Note: this is not needed on X86 for Visual Studio, as the called function
		// destroys the values passed (don't ask me how it works with variable argument lists).
		// This implementation is provided as a reference implementation.
		byte *at = (byte *)to;
		for (nat i = 0; i < params.size(); i++) {
			const Param &p = params[i];
			nat s = roundUp(p.size, sizeof(void *));
			if (p.destroy)
				(*p.destroy)(at);
			at += s;
		}
	}

	void FnCall::doCall4(void *result, const void *fn) {
		nat sz = paramsSize();
		__asm {
			// Ebx is preserved along function calls!
			mov ebx, sz;
			// ecx is the 'this' ptr in the call to 'copyParams'
			mov ecx, this;

			sub esp, ebx;
			mov eax, esp;

			// Call copyParams
			push eax;
			call copyParams;

			call fn;
			add esp, ebx;

			// Now it should be safe to access stack-allocated variables again!
			mov ebx, result;
			mov [ebx], eax;
		}
	}

	void FnCall::doCall8(void *result, const void *fn) {
		nat sz = paramsSize();
		__asm {
			mov ebx, sz;
			mov ecx, this;

			sub esp, ebx;
			mov eax, esp;

			push eax;
			call copyParams;

			call fn;
			add esp, ebx;

			mov ebx, result;
			mov [ebx], eax;
			mov [ebx+4], edx;
		}
	}

	void FnCall::doCallLarge(void *result, nat retSz, const void *fn) {
		nat sz = paramsSize();
		__asm {
			mov ebx, sz;
			mov ecx, this;

			sub esp, ebx;
			mov eax, esp;

			push eax;
			call copyParams;

			push result;
			call fn;
			add esp, 4;
			add esp, sz;
		}
	}

	void FnCall::doCall(void *result, const TypeInfo &info, const void *fn) {
		nat s = roundUp(info.size, sizeof(void *));

		if (info.plain() && info.kind == TypeInfo::user) {
			// Result is returned on stack.
			doCallLarge(result, s, fn);
			return;
		}

		if (s <= 4)
			doCall4(result, fn);
		else if (s <= 8)
			doCall8(result, fn);
		else
			doCallLarge(result, s, fn);
	}

	void FnCall::callVoid(const void *fn) {
		nat dummy;
		doCall4(&dummy, fn);
	}

	float FnCall::doFloatCall(const void *fn) {
		return (float)doDoubleCall(fn);
	}

	double FnCall::doDoubleCall(const void *fn) {
		nat sz = paramsSize();
		__asm {
			mov ebx, sz;
			mov ecx, this;

			sub esp, ebx;
			mov eax, esp;

			push eax;
			call copyParams;

			call fn;
			add esp, ebx;
		}
		// we're fine! Output is already stored on the floating-point stack.
	}

#endif

}
