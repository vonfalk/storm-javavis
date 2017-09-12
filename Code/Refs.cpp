#include "stdafx.h"
#include "Refs.h"
#include "Core/GcCode.h"
#include "X86/Refs.h"
#include "X64/Refs.h"

namespace code {

#if defined(X86)
#define ARCH x86
#elif defined(X64)
#define ARCH x64
#else
#error "Writing pointers is not supported for your architecture yet!"
#endif

	void updatePtrs(void *code, const GcCode *refs) {
		for (Nat i = 0; i < refs->refCount; i++)
			ARCH::writePtr(code, refs, i, size_t(refs->refs[i].pointer));
	}

	void writePtr(void *code, Nat id) {
		GcCode *refs = runtime::codeRefs(code);
		ARCH::writePtr(code, refs, id, size_t(refs->refs[id].pointer));
	}

	void writePtr(void *code, Nat id, void *ptr) {
		ARCH::writePtr(code, runtime::codeRefs(code), id, size_t(ptr));
	}

}
