#include "stdafx.h"
#include "Refs.h"
#include "Core/GcCode.h"

namespace code {
	namespace x86 {

		void writePtr(void *code, const GcCode *refs, Nat id) {
			// Nothing special here!
			dbg_assert(false, L"Unknown pointer type.");
		}

	}
}
