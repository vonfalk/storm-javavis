#include "stdafx.h"
#include "VTableStorm.h"
#include "VTableCpp.h"

namespace storm {
	using code::MemberRef;

	VTableStorm::VTableStorm(VTableCpp *update) : update(update) {}

	void VTableStorm::clear() {
		table = null;
		refs = null;
	}

	void VTableStorm::resize(Nat count) {
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
		GcArray<MemberRef *> *m = runtime::allocArray<MemberRef *>(engine(), &pointerArrayType, size);
		n->filled = table ? table->filled : 0;
		m->filled = refs ? refs->filled : 0;
		for (nat i = 0; i < n->filled; i++) {
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

	Nat VTableStorm::findSlot(Function *fn) const {
		const void *find = fn->directRef()->address();

		for (nat i = 0; i < count(); i++) {
			if (refs->v[i] != null && table->v[i] == find)
				return i;
		}

		return vtable::invalid;
	}

	void VTableStorm::insert(Nat pos, Nat ins) {
		resize(count() + ins);

		for (nat i = count(); i > pos; i--) {
			nat from = i - 1;
			nat to = from + ins;

			table->v[to] = table->v[from];
			refs->v[to] = refs->v[from];
			if (refs->v[to])
				refs->v[to]->move(table, to);

			table->v[from] = null;
			refs->v[from] = null;
		}
	}

	void VTableStorm::set(Nat slot, Function *fn, code::Content *from) {
		assert(slot < count());
		if (refs->v[slot])
			refs->v[slot]->disable();
		refs->v[slot] = new (this) code::MemberRef(table, slot, fn->directRef(), from);
	}

	void VTableStorm::clear(Nat slot) {
		assert(slot < count());
		if (refs->v[slot])
			refs->v[slot]->disable();
		refs->v[slot] = null;
		table->v[slot] = null;
	}

}
