#pragma once
#include "Core/GcCode.h"

namespace code {

	// Update references in a code segment. This code is called from the garbage collector, so this
	// code may only look at the code segment provided and nothing more. No calls to the GC are
	// allowed either.
	void updatePtrs(void *code, const GcCode *refs);

	// Write pointers from a code allocation.
	void writePtr(void *code, Nat id);
	void writePtr(void *code, Nat id, void *ptr);

	// Do we require finalization of code segments?
	bool needFinalization();

	// If so, call this function when required.
	void finalize(void *code);

}
