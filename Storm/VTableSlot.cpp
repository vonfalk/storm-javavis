#include "stdafx.h"
#include "VTableSlot.h"
#include "VTable.h"
#include "Function.h"

namespace storm {

	VTableSlot::VTableSlot(VTable &owner, Function *fn, VTablePos pos)
		: Reference(fn->directRef(), L"vtable"), owner(owner), fn(fn), id(pos) {}

	void VTableSlot::onAddressChanged(void *na) {
		Reference::onAddressChanged(na);
		update();
	}

	void VTableSlot::update() {
		owner.updateAddr(id, address(), this);
	}


}
