#pragma once

namespace code {
	namespace x86 {

		// Writing references for X86.
		void writePtr(void *code, const GcCode *refs, Nat id);

		// No finalization for X86.
		inline bool needFinalization() { return true; }
		void finalize(void *code);

	}
}
