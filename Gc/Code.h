#pragma once

namespace storm {
	namespace code {

		/**
		 * Interfaces for managing code allocations in the GC implementations.
		 *
		 * These functions are called by the GC, so the implementation may only look at the provided
		 * code segment and nothing more (at least nothing else managed by the GC). No calls to the
		 * GC are allowed either, except for calls to static functions that read information from
		 * the code segment, such as "codeRefs" and "codeSize".
		 */

		// Update all references in the specified code segment. This is called after the code
		// segment has been initialized, but also whenever the code segment has been moved.
		void updatePtrs(void *code, const GcCode *refs);

		// Write a specific pointer to a code allocation.
		void writePtr(void *code, Nat id);

		// Do code segments require finalization?
		Bool needFinalization();

		// If so, call this function when finalizing a code segment.
		void finalize(void *code);

	}
}
