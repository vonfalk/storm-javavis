#pragma once

namespace code {
	namespace x64 {

		// Reading/writing references for X86-64.
		size_t readPtr(void *code, const GcCode *refs, Nat id);
		void writePtr(void *code, const GcCode *refs, Nat id, size_t ptr);

	}
}
