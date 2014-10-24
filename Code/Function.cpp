#include "stdafx.h"
#include "Function.h"

namespace code {

#ifdef X86

	void *fnCall(void *fnPtr, nat paramCount, const void **params) {
		nat paramSize = paramCount * sizeof(void *);
		void *result;
		const void **stack;
		__asm mov stack, esp;

		const void **to = stack - paramCount;
		for (nat i = 0; i < paramCount; i++)
			to[i] = params[i];
		// Excercise: Why is this a bad idea?
		// memcpy(sptr - paramSize, params, paramSize);

		__asm {
			sub esp, paramSize;
			call fnPtr;
			add esp, paramSize;
			mov result, eax;
		}

		return result;
	}

#endif

}
