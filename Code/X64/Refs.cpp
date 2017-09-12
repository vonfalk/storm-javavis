#include "stdafx.h"
#include "Refs.h"
#include "Core/GcCode.h"

namespace code {
	namespace x64 {

		void writePtr(void *code, const GcCode *refs, Nat id) {
			// Nothing special here yet!
			dbg_assert(false, L"Unknown pointer type.");
		}

	}
}
