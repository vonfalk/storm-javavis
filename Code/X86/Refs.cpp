#include "stdafx.h"
#include "Refs.h"
#include "Core/GcCode.h"

namespace code {
	namespace x86 {

		size_t readPtr(void *code, const GcCode *refs, Nat id) {
			const GcCodeRef &ref = refs->refs[id];
			void *readVoid = ((byte *)code) + ref.offset;
			size_t *read = (size_t *)readVoid;

			switch (ref.kind) {
			case GcCodeRef::disabled:
				return 0;
			case GcCodeRef::rawPtr:
				return *read;
			case GcCodeRef::relativePtr:
			case GcCodeRef::relative:
				return *read + size_t(read) + sizeof(size_t);
			case GcCodeRef::inside:
				return *read + size_t(code);
			default:
				dbg_assert(false, L"Unknown pointer type.");
				return 0;
			}
		}

		void writePtr(void *code, const GcCode *refs, Nat id, size_t ptr) {
			const GcCodeRef &ref = refs->refs[id];
			void *writeVoid = ((byte *)code) + ref.offset;
			size_t *write = (size_t *)writeVoid;

			// Note: These writes need to be visible as an atomic operation to the instruction
			// decoding unit of the CPU. Further atomicity is not required, as the GC arranges for
			// that anyway. Offsets may not be aligned as regular pointers on the system.

			switch (ref.kind) {
			case GcCodeRef::disabled:
				// Nothing to do...
				break;
			case GcCodeRef::rawPtr:
				unalignedAtomicWrite(*write, ptr);
				break;
			case GcCodeRef::relativePtr:
			case GcCodeRef::relative:
				unalignedAtomicWrite(*write, ptr - (size_t(write) + sizeof(size_t)));
				break;
			case GcCodeRef::inside:
				unalignedAtomicWrite(*write, ptr + size_t(code));
				break;
			default:
				dbg_assert(false, L"Unknown pointer type.");
				break;
			}
		}

	}
}
