#include "stdafx.h"
#include "VTable.h"

namespace storm {

	VTable::VTable() {}

	void VTable::createCpp(const void *cppVTable) {
		assert(original == null, L"Can not re-use a VTable for a new C++ root object.");
		original = cppVTable;
	}

	void VTable::createStorm(VTable *parent) {
		original = parent->original;

		if (parent->cpp) {
			// Reuse the previously computed size if we're able to.
			if (cpp) {
				cpp->replace(original, parent->cpp->count());
			} else {
				cpp = new (this) VTableCpp(original, parent->cpp->count());
			}
		} else {
			if (cpp) {
				cpp->replace(original);
			} else {
				cpp = new (this) VTableCpp(original);
			}
		}

		// Create a new Storm VTable.
		storm = new (this) VTableStorm(cpp);
	}

}
