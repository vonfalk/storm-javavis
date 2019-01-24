#pragma once
#include "Core/TObject.h"
#include "Code/Reference.h"
#include "Code/Listing.h"
#include "Gc/VTable.h"

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

		// Get number of elements. Note: 'size' returns number of bytes.
		nat count() const;

		// Set this VTable for a class.
		void insert(RootObject *obj);

		// Generate code to set VTable for a class. 'vtableRef' is a reference to the VTableCpp used.
		static void insert(code::Listing *to, code::Var obj, code::Ref vtableRef);

		// Get the offset between the vtable allocation and the actual start of the vtable.
		static size_t vtableAllocOffset() { return vtable::allocOffset(); }

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

}
