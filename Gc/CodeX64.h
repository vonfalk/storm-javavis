#pragma once

namespace storm {
	namespace x64 {

		// Writing references for X86-64.
		void writePtr(void *code, const GcCode *refs, Nat id);

		// Finalization for X86-64.
		inline bool needFinalization() { return true; }
		void finalize(void *code);

	}
}
