#include "stdafx.h"
#include "Refs.h"
#include "Asm.h"
#include "Core/GcCode.h"

namespace code {
	namespace x86 {

		void writePtr(void *code, const GcCode *refs, Nat id) {
			const GcCodeRef &ref = refs->refs[id];
			switch (ref.kind) {
			case GcCodeRef::unwindInfo:
				if (ref.pointer)
					CodeTable::update(ref.pointer, code);
				break;
			default:
				dbg_assert(false, L"Unknown pointer type.");
				break;
			}
		}

		void finalize(void *code) {
			GcCode *refs = runtime::codeRefs(code);
			for (size_t i = 0; i < refs->refCount; i++) {
				GcCodeRef &ref = refs->refs[i];
				if (ref.kind == GcCodeRef::unwindInfo && ref.pointer) {
					CodeTable::Handle h = ref.pointer;

					// Disable future updates:
					atomicWrite(ref.pointer, null);
					codeTable().remove(h);
				}
			}
		}

	}
}
