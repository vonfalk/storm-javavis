#pragma once
#include "Core/TObject.h"
#include "Code/Reference.h"
#include "Code/Listing.h"

namespace storm {
	STORM_PKG(core.lang);

	class Function;

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
	class VTableCpp : public code::Content {
		STORM_CLASS;
	public:
		// Create a VTable which is a write-protected wrapper around the raw C++ vtable.
		static VTableCpp *wrap(Engine &e, const void *vtable);
		static VTableCpp *wrap(Engine &e, const void *vtable, nat count);

		// Create a VTable which is a copy of the provided C++ vtable or another VTableCpp object.
		static VTableCpp *copy(Engine &e, const void *vtable);
		static VTableCpp *copy(Engine &e, const void *vtable, nat count);
		static VTableCpp *copy(Engine &e, const VTableCpp *src);

		// Get our size.
		nat count() const;

		// Set this VTable for a class.
		void insert(RootObject *obj);

		// Generate code to set VTable for a class. 'vtableRef' is a reference to the VTableCpp used.
		static void insert(code::Listing *to, code::Var obj, code::Ref vtableRef);

		// Get the offset between the vtable allocation and the actual start of the vtable.
		static size_t vtableAllocOffset();

		// Replace the contents in here with a new vtable. Clears all references.
		void replace(const void *vtable);
		void replace(const void *vtable, nat count);
		void replace(const VTableCpp *src);

		// Get/set the extra data. Not present if write-protected.
		const void *extra() const;
		void extra(const void *to);

		// Find a function in here. Returns vtable::invalid if none is found.
		nat findSlot(const void *fn) const;

		// Set a slot. When write protected, this is a no-op.
		void set(nat slot, const void *to);
		void set(nat slot, Function *fn);

		// Get the function associated with a vtable entry.
		MAYBE(Function *) get(nat slot) const;

		// Clear a vtable entry.
		void clear(nat slot);

		virtual const void *address() const;
		virtual nat size() const;

	private:
		// Create.
		VTableCpp(const void *src, nat count, bool copy);

		// The VTable data. We're storing it as a regular GC array. May be null if we're referring
		// to a raw c++ vtable. The 'filled' member of this array is used to indicate that the
		// vtable has been assigned to an object at some time.
		GcArray<const void *> *data;

		// References updating the VTable. The entire table is null if no updaters are added.
		GcArray<Function *> *refs;

		// Raw write-protected C++ vtable.
		const void **raw;

		// Size of the vtable in here.
		nat tabSize;

		// Initialize ourselves.
		void init(const void *vtable, nat count, bool copy);

		// Get a pointer to the start of the vtable.
		const void **table() const;

		// Is the table used?
		inline bool used() const;
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
		const void *from(const RootObject *object);

		// Set the current vtable for an object.
		void set(const void *vtable, RootObject *object);

	}
}
