#pragma once

namespace code {

	// Update references in a code segment. This code is called from the garbage collector, so this
	// code may only look at the code segment provided and nothing more. No calls to the GC are
	// allowed either.
	void updatePtrs(void *code, const GcCode *refs);

	// Read/write pointers from a code allocation.
	void *readPtr(void *code, Nat id);
	void writePtr(void *code, Nat id, void *ptr);

}
