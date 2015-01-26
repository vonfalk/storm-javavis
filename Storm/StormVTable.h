#pragma once
#include "Code/Reference.h"

namespace code {
	class VTable;
}

namespace storm {

	class VTable;
	class Function;

	/**
	 * Source for the VTable references.
	 */
	class VTableUpdater : public code::Reference {
		friend class StormVTable;
	public:
		VTableUpdater(VTable &owner, Function *fn);

		// Called when updated.
		virtual void onAddressChanged(void *addr);

		// Update the vtable now.
		void update();

		// The actual function.
		Function *const fn;

	private:
		// Which VTable to update?
		VTable &owner;

		// The index to update.
		nat id;
	};

	/**
	 * Helper for handling the raw array in which we are storing the Storm VTable.
	 */
	class StormVTable : NoCopy {
	public:
		// Ctor.
		StormVTable(code::VTable *&update);

		// Dtor.
		~StormVTable();

		// Clear entire contents.
		void clear();

		// Ensure a specific size.
		void ensure(nat size);

		// Get/set an item.
		void item(nat id, VTableUpdater *data);
		VTableUpdater *item(nat id);

		// Get/set an address. Assumes that it is not an item.
		void addr(nat id, void *ptr);
		void *addr(nat id);

		// Find an empty item. Returns the count() if there is none.
		nat emptyItem(nat from = 0) const;

		// Find an addr equal to the one given. Returns count() if none.
		nat findAddr(void *addr) const;

		// Find an item. Returns count() if none.
		nat findItem(VTableUpdater *item) const;
		nat findItem(Function *item) const;

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
		VTableUpdater **src;

		// Size.
		nat size;

		// Capacity.
		nat capacity;
	};


}
