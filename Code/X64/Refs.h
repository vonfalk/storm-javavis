#pragma once

namespace code {
	namespace x64 {

		// Writing references for X86-64.
		void writePtr(void *code, const GcCode *refs, Nat id, size_t ptr);

	}
}
