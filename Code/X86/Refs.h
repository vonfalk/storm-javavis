#pragma once

namespace code {
	namespace x86 {

		// Writing references for X86.
		void writePtr(void *code, const GcCode *refs, Nat id);

	}
}
