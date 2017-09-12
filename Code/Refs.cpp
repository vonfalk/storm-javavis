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

	// The generic cases. These are implemented the same on every platform!
	static void doWritePtr(void *code, const GcCode *refs, Nat id) {
		const GcCodeRef &ref = refs->refs[id];
		size_t ptr = size_t(ref.pointer);
		void *write = ((byte *)code) + ref.offset;

		// Note: These writes need to be visible as an atomic operation to the instruction
		// decoding unit of the CPU. Further atomicity is not required, as the GC arranges for
		// that anyway. Offsets may not be aligned as regular pointers on the system.

		switch (ref.kind) {
		case GcCodeRef::disabled:
			// Nothing to do...
			break;
		case GcCodeRef::rawPtr:
			unalignedAtomicWrite(*(size_t *)write, ptr);
			break;
		case GcCodeRef::relativePtr:
		case GcCodeRef::relative:
			unalignedAtomicWrite(*(size_t *)write, ptr - (size_t(write) + sizeof(size_t)));
			break;
		case GcCodeRef::inside:
			unalignedAtomicWrite(*(size_t *)write, ptr + size_t(code));
			break;
		case GcCodeRef::relativeHere:
			// Write a relative pointer to 'pointer' itself. Should always be the same, but the
			// offset is not really exposed conveniently anywhere else.
			ptr = size_t(&ref.pointer);
			shortUnalignedAtomicWrite(*(nat *)write, nat(ptr - (size_t(write) + sizeof(nat))));
			break;
		default:
			// Pass on to the architecture specific parts:
			ARCH::writePtr(code, refs, id);
			break;
		}
	}

	void updatePtrs(void *code, const GcCode *refs) {
		for (Nat i = 0; i < refs->refCount; i++)
			doWritePtr(code, refs, i);
	}

	void writePtr(void *code, Nat id) {
		GcCode *refs = runtime::codeRefs(code);
		doWritePtr(code, refs, id);
	}

	void writePtr(void *code, Nat id, void *ptr) {
		doWritePtr(code, runtime::codeRefs(code), id);
	}

}
