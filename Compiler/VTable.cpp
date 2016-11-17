#include "stdafx.h"
#include "VTable.h"
#include "Type.h"

namespace storm {

	VTable::VTable() {}

	void VTable::createCpp(const void *cppVTable) {
		assert(original == null, L"Can not re-use a VTable for a new C++ root object.");
		original = cppVTable;
		stormFirst = 1;
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
		stormFirst = parent->storm
			? parent->storm->count()
			: 1; // always place stuff after the destructor slot.
	}

	VTableSlot VTable::findSlot(Function *fn) {
		Code *c = fn->getCode();
		if (as<StaticCode>(c)) {
			// Might be a function from C++.
			nat r = cpp
				? cpp->findSlot(fn->directRef()->address())
				: vtable::find(original, fn->directRef()->address());

			if (r != vtable::invalid)
				return cppSlot(r);
		}

		if (storm)
			return stormSlot(storm->findSlot(fn));
		else
			return VTableSlot();
	}

	void VTable::set(VTableSlot slot, Function *fn, code::Content *from) {
		if (!slot.valid()) {
			WARNING(L"Ignoring invalid slot for " << fn);
			return;
		}

		if (cpp == null || storm == null) {
			// Can not alter a C++ vtable.
			return;
		}

		switch (slot.type) {
		case VTableSlot::tCpp:
			cpp->set(slot.offset, fn, from);
			break;
		case VTableSlot::tStorm:
			storm->set(slot.offset, fn, from);
			break;
		default:
			assert(false, L"Unknown slot type.");
			break;
		}
	}

	VTableSlot VTable::createStorm(Type *type) {
		if (storm == null) {
			assert(false);
			return VTableSlot();
		}

		nat slot = storm->freeSlot(stormFirst);
		if (slot >= storm->count()) {
			// We need to resize 'storm'...
			storm->resize(storm->count() + stormGrow);

			// Tell the VTables of all children to grow...
			type->vtableGrow(storm->count());
		}

		return cppSlot(slot);
	}

	void VTable::parentGrown(Nat parentCount) {
		if (storm)
			storm->insert(stormFirst, parentCount - stormFirst);
		stormFirst = parentCount;
	}

}
