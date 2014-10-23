#include "stdafx.h"
#include "Function.h"

namespace code {

#ifdef X86

	void *fnCall(void *fnPtr, nat paramCount, const void **params) {
		byte *sptr;
		__asm mov sptr, esp;
		nat paramSize = paramCount * sizeof(void *);
		memcpy(sptr - paramSize, params, paramSize);
		__asm {
			sub esp, paramSize;
			call fnPtr;
			add esp, paramSize;
		}
	}

#endif

}
