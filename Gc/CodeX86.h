#pragma once

namespace storm {
	namespace x86 {

		// Writing references for X86.
		void writePtr(void *code, const GcCode *refs, Nat id);

		// Finalization.
		inline Bool needFinalization() { return true; }
		void finalize(void *code);
	}
}
