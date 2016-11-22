#include "stdafx.h"
#include "VTableUpdater.h"
#include "VTable.h"

namespace storm {

	VTableUpdater::VTableUpdater(VTable *table, VTableSlot slot, code::Ref ref, code::Content *from)
		: code::Reference(ref, from), table(table), slot(slot) {

		moved(address());
	}

	void VTableUpdater::moved(const void *newAddr) {
		if (!table)
			return;
		if (!slot.valid())
			return;

		table->slotMoved(slot, newAddr);
	}

	void VTableUpdater::disable() {
		table = null;
		slot = VTableSlot();
	}

}
