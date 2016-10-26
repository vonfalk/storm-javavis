#include "stdafx.h"
#include "VTableStorm.h"
#include "VTableCpp.h"

namespace storm {
	using code::Reference;

	VTableStorm::VTableStorm(VTableCpp *update) : update(update) {}

	void VTableStorm::clear() {
		table = null;
		refs = null;
	}

	void VTableStorm::ensure(Nat count) {
		if (table != null && count <= table->count) {
			// Release possible GC retention.
			for (nat i = min(table->filled, count); i < table->count; i++) {
				table->v[i] = null;
				refs->v[i] = null;
			}
			table->filled = count;

			// Note: we never shrink as of now.
			return;
		}

		// Grow! Note: growing 10 elements at a time as opposed to quadratic.
		nat size = table ? table->count : 0;
		size = max(count, size + 10);

		GcArray<const void *> *n = runtime::allocArray<const void *>(engine(), &pointerArrayType, size);
		GcArray<Reference *> *m = runtime::allocArray<Reference *>(engine(), &pointerArrayType, size);
		n->filled = table->filled;
		m->filled = refs->filled;
		for (nat i = 0; i < table->filled; i++) {
			n->v[i] = table->v[i];
			m->v[i] = refs->v[i];
		}

		table = n;
		update->extra() = table;
	}

	Nat VTableStorm::count() const {
		return table ? table->filled : 0;
	}

	Nat VTableStorm::dtorSlot() const {
		return 0;
	}

	Nat VTableStorm::freeSlot() const {
		return freeSlot(0);
	}

	Nat VTableStorm::freeSlot(Nat first) const {
		for (nat i = first; i < count(); i++) {
			if (refs->v[i] == null)
				return i;
		}

		return count();
	}

}
