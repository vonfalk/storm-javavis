#pragma once
#include "Code/Reference.h"
#include "VTableSlot.h"

namespace storm {
	STORM_PKG(core.lang);

	class VTable;

	/**
	 * Reference which update entries in VTables.
	 */
	class VTableUpdater : public code::Reference {
		STORM_CLASS;
	public:
		// Update 'slot' inside the vtable with 'fn'.
		VTableUpdater(VTable *table, VTableSlot slot, code::Ref ref, code::Content *from);

		// Notification of changed address.
		virtual void moved(const void *newAddr);

		// Disable updating.
		void STORM_FN disable();

		// Get the slot.
		VTableSlot STORM_FN slot();

		// Set the slot.
		void STORM_FN slot(VTableSlot slot);

	private:
		// VTable we are to update.
		VTable *table;

		// Slot to update.
		VTableSlot mySlot;
	};

}
