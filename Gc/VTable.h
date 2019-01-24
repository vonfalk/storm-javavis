#pragma once
#include "Utils/Memory.h"
#include "Core/GcArray.h"

namespace storm {

	/**
	 * Low-level layout for VTables, so that the GC can scan the dynamically allocated parts
	 * properly.
	 */
	namespace vtable {

		// The offset for the extra data in a vtable. Varies between platforms, but is always
		// negative, so we only store the magnitude here.
		extern const Nat extraOffset;

		// Offset for the destructor. There is no way to get a pointer to the destructor, so it is
		// treated specially.
		extern const Nat dtorOffset;

		// Invalid slot id.
		extern const Nat invalid;

		// Offset (in bytes) from the vtable allocation to the first vtable entry in the VTable.  GC
		// implementations need to consider this to find the pointer of the actual allocation from
		// the VTable pointer stored in the object.
		static inline size_t allocOffset() {
			return OFFSET_OF(GcArray<const void *>, v[extraOffset]);
		}

		// De-virtualize a function call. When taking the pointer of a virtual function, one gets a
		// pointer to a small virtual dispatch stub, not the actual implementation for the specified
		// subclass (which is reasonable). Sometimes, however, we need to know which function is
		// actually called for a specific subclass. This function looks up what the function would call
		// and returns the address of that function.
		const void *deVirtualize(const void *vtable, const void *fnPtr);

		// Find the slot for 'fn' in 'vtable'. 'fn' may or may not be devirtualized.
		Nat find(const void *vtable, const void *fn);
		Nat find(const void *vtable, const void *fn, Nat size);

		// Find the slot which 'fn' references. Assumes 'fn' has not yet been devirtualized.
		Nat fnSlot(const void *fn);

		// Get a slot in a vtable.
		const void *slot(const void *vtable, Nat slot);

		// Compute the (approximate) size of a vtable.
		Nat count(const void *vtable);

		// Get the current vtable from an object.
		const void *from(const RootObject *object);

		// Set the current vtable for an object.
		void set(const void *vtable, RootObject *object);

	}

}
