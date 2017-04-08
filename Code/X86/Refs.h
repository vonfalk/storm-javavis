#pragma once

namespace code {
	namespace x86 {

		// Reading/writing references for X86.
		size_t readPtr(void *code, const GcCode *refs, Nat id);
		void writePtr(void *code, const GcCode *refs, Nat id, size_t ptr);

	}
}
