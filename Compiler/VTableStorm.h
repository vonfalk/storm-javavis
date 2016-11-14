#pragma once
#include "Core/TObject.h"
#include "Core/GcArray.h"
#include "Code/Reference.h"
#include "Function.h"

namespace storm {
	STORM_PKG(core.lang);

	class VTableCpp;

	/**
	 * Represents all VTable entries for Storm. The first entry is always reserved for Storm's
	 * destructor.
	 */
	class VTableStorm : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		// Create, give the C++ vtable which we have to update.
		STORM_CTOR VTableStorm(VTableCpp *update);

		// Clear entirely.
		void STORM_FN clear();

		// Ensure we're at least a specific size.
		void STORM_FN ensure(Nat count);

		// Current # of elements.
		Nat STORM_FN count() const;

		// Get the slot reserved for the destructor.
		Nat STORM_FN dtorSlot() const;

		// Find an empty slot, optionally starting at a specific index. If no slot is free, returns size().
		Nat STORM_FN freeSlot() const;
		Nat STORM_FN freeSlot(Nat first) const;

		// Find the slot used for a specific function. Returns vtable::invalid if none is found.
		Nat STORM_FN findSlot(Function *fn) const;

		// Set a slot.
		void STORM_FN set(Nat slot, Function *fn, code::Content *from);

		// Get a pointer to the table.
		inline const void *ptr() const { return table; }

	private:
		// The VTable itself.
		GcArray<const void *> *table;

		// References updating the VTable.
		GcArray<code::Reference *> *refs;

		// Update this VTable.
		VTableCpp *update;
	};

}
