#pragma once
#include "VTableSlot.h"

namespace code {
	class VTable;
}

namespace storm {

	class Function;

	/**
	 * Helper for handling the raw array in which we are storing the Storm VTable.
	 * Note: the first entry is reserved for Storm's destructor.
	 */
	class StormVTable : NoCopy {
	public:
		// Ctor.
		StormVTable(code::VTable *&update);

		// Dtor.
		~StormVTable();

		// Clear entire contents.
		void clear();

		// Clear references.
		void clearRefs();

		// Ensure a specific size.
		void ensure(nat size);

		// Get/set an slot.
		void slot(nat id, VTableSlot *data);
		VTableSlot *slot(nat id);

		// Get/set an address. Assumes that it is not an slot.
		void addr(nat id, void *ptr);
		void *addr(nat id);

		// Find an empty slot. Returns the count() if there is none.
		nat emptySlot(nat from = 0) const;

		// Find an addr equal to the one given. Returns count() if none.
		nat findAddr(void *addr) const;

		// Find an slot. Returns count() if none.
		nat findSlot(VTableSlot *slot) const;
		nat findSlot(Function *slot) const;

		// Current size.
		inline nat count() const { return size; }

		// Expand the vector by inserting 'null' at 'at', 'count' times.
		void expand(nat at, nat count);

		// Contract the vector by removing [from, to[.
		void contract(nat from, nat to);

	private:
		// What to update with our ptr.
		code::VTable *&update;

		// The raw vtable.
		void **addrs;

		// The source of addresses to the vtable.
		VTableSlot **src;

		// Size.
		nat size;

		// Capacity.
		nat capacity;

		// Dump.
		void dbg_dump();
	};


}
