#pragma once
#include "Core/TObject.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * This file manages a C++ VTable. This is done in a way which is compatible with the C++
	 * compiler implementation, so that we can extend classes in C++ in Storm seamlessly.
	 *
	 * Note: as we need to be able to access a negative index of the table, and since we can not have
	 * internal pointers to objects due to the GC, we actually always point to index -extraOffset in the
	 * VTable when storing it in this class.
	 *
	 * Note: This class assumes that all objects in C++ only use single inheritance.
	 */
	class VTableCpp : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
	};


	/**
	 * Low-level helpers for VTables.
	 */
	namespace vtable {

		// The offset for the extra data in the VTable. Varies between platforms, but is always negative.
		extern const nat extraOffset;

		// Offset for the destructor. There is no way to get a pointer to the destructor, so it is
		// treated specially.
		extern const nat dtorOffset;

		// Invalid slot id.
		extern const nat invalid;

		// De-virtualize a function call. When taking the pointer of a virtual function, one gets a
		// pointer to a small virtual dispatch stub, not the actual implementation for the specified
		// subclass (which is reasonable). Sometimes, however, we need to know which function is
		// actually called for a specific subclass. This function looks up what the function would call
		// and returns the address of that function.
		const void *deVirtualize(const void *vtable, const void *fnPtr);

		// Find the slot for 'fn' in 'vtable'. 'fn' may or may not be devirtualized.
		nat find(const void *vtable, const void *fn);
		nat find(const void *vtable, const void *fn, nat size);

		// Find the slot which 'fn' references. Assumes 'fn' has not yet been devirtualized.
		nat fnSlot(const void *fn);

		// Get a slot in a vtable.
		const void *slot(const void *vtable, nat slot);

		// Compute the (approximate) size of a vtable.
		nat count(const void *vtable);

		// Get the current vtable from an object.
		const void *from(const void *object);

		// Set the current vtable for an object.
		void set(const void *vtable, void *object);

	}
}
