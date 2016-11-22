#include "stdafx.h"
#include "VTableUpdater.h"
#include "VTable.h"

namespace storm {

	VTableUpdater::VTableUpdater(VTable *table, VTableSlot slot, code::Ref ref, code::Content *from)
		: code::Reference(ref, from), table(table), mySlot(slot) {

		moved(address());
	}

	void VTableUpdater::moved(const void *newAddr) {
		if (!table)
			return;
		if (!mySlot.valid())
			return;

		table->slotMoved(mySlot, newAddr);
	}

	void VTableUpdater::disable() {
		table = null;
		mySlot = VTableSlot();
	}

	VTableSlot VTableUpdater::slot() {
		return mySlot;
	}

}
