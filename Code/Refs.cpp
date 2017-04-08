#include "stdafx.h"
#include "Refs.h"
#include "Core/GcCode.h"
#include "X86/Refs.h"

namespace code {

#ifdef X86
#define ARCH x86
#else
#error "Reading and writing pointers is not supported for your architecture yet!"
#endif

	void updatePtrs(void *code, const GcCode *refs) {
		for (Nat i = 0; i < refs->refCount; i++)
			ARCH::writePtr(code, refs, i, size_t(refs->refs[i].pointer));
	}

	void *readPtr(void *code, Nat id) {
		return (void *)ARCH::readPtr(code, runtime::codeRefs(code), id);
	}

	void writePtr(void *code, Nat id, void *ptr) {
		ARCH::writePtr(code, runtime::codeRefs(code), id, size_t(ptr));
	}

}
