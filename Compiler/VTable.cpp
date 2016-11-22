#include "stdafx.h"
#include "VTable.h"
#include "Type.h"
#include "Engine.h"

namespace storm {

	VTable::VTable(Type *owner) : owner(owner) {}

	void VTable::createCpp(const void *cppVTable) {
		assert(original == null, L"Can not re-use a VTable for a new C++ root object.");
		original = cppVTable;
		stormFirst = 1;

		if (engine().has(bootTemplates))
			lateInit();
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
		if (source)
			source->set(cpp);

		// Create a new Storm VTable.
		storm = new (this) VTableStorm(cpp);
		stormFirst = parent->storm
			? parent->storm->count()
			: 1; // always place stuff after the destructor slot.

		if (engine().has(bootTemplates))
			lateInit();
	}

	void VTable::lateInit() {
		if (!updaters)
			updaters = new (this) Map<Function *, VTableUpdater *>();

		if (!source) {
			source = new (this) code::RefSource(L"vtable");

			if (cpp)
				source->set(cpp);
			else
				source->setPtr(original);
		}
	}

	void VTable::insert(Function *fn) {
		return;

		VTableSlot slot = findSlot(fn);

		set(slot, fn);
		updaters->put(fn, new (this) VTableUpdater(this, slot, fn->directRef(), source->content()));

		// TODO: We need to find functions in parent classes where we need to enable vtable lookup.
	}

	void VTable::removeChild(Type *child) {
		TODO(L"Implement me!");
	}

	void VTable::slotMoved(VTableSlot slot, const void *addr) {
		set(slot, addr);

		// Traverse the types and update all child types.
		if (!owner->chain)
			return;

		Array<Type *> *c = owner->chain->children();
		for (nat i = 0; i < c->count(); i++) {
			if (VTable *t = c->at(i)->vtable)
				t->parentSlotMoved(slot, addr);
		}
	}

	void VTable::parentSlotMoved(VTableSlot slot, const void *addr) {
		// We do not need to propagate further.
		if (get(slot) != null)
			return;

		if (!owner->chain)
			return;

		Array<Type *> *c = owner->chain->children();
		for (nat i = 0; i < c->count(); i++) {
			if (VTable *t = c->at(i)->vtable)
				t->parentSlotMoved(slot, addr);
		}
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

	void VTable::set(VTableSlot slot, Function *fn) {
		if (cpp == null || storm == null) {
			// Can not alter a C++ vtable.
			return;
		}

		switch (slot.type) {
		case VTableSlot::tCpp:
			cpp->set(slot.offset, fn);
			break;
		case VTableSlot::tStorm:
			storm->set(slot.offset, fn);
			break;
		default:
			assert(false, L"Unknown slot type.");
			break;
		}
	}

	void VTable::set(VTableSlot slot, const void *addr) {
		if (cpp == null || storm == null)
			return;

		switch (slot.type) {
		case VTableSlot::tCpp:
			cpp->slot(slot.offset) = addr;
			break;
		case VTableSlot::tStorm:
			storm->set(slot.offset, addr);
			break;
		default:
			assert(false, L"Unknown slot type.");
			break;
		}
	}

	Function *VTable::get(VTableSlot slot) {
		if (cpp == null || storm == null)
			return null;

		switch (slot.type) {
		case VTableSlot::tCpp:
			return cpp->get(slot.offset);
		case VTableSlot::tStorm:
			return storm->get(slot.offset);
		default:
			assert(false, L"Unknown slot type.");
			return null;
		}
	}

	void VTable::clear(VTableSlot slot) {
		if (cpp == null || storm == null)
			return;

		switch (slot.type) {
		case VTableSlot::tCpp:
			cpp->clear(slot.offset);
			break;
		case VTableSlot::tStorm:
			storm->clear(slot.offset);
			break;
		default:
			assert(false, L"Unknown slot type.");
			break;
		}
	}

	VTableSlot VTable::createStorm() {
		if (storm == null) {
			assert(false);
			return VTableSlot();
		}

		nat slot = storm->freeSlot(stormFirst);
		if (slot >= storm->count()) {
			nat oldCount = storm->count();

			// We need to resize 'storm'...
			storm->resize(oldCount + stormGrow);

			// Tell the VTables of all children to grow...
			owner->vtableGrow(oldCount, stormGrow);
		}

		return cppSlot(slot);
	}

	void VTable::parentGrown(Nat pos, Nat count) {
		if (storm)
			storm->insert(pos, count);
		if (pos >= stormFirst)
			stormFirst += count;
	}




	// NOTE: Slightly dangerous to re-use the parameters from the function...
	OverridePart::OverridePart(Function *src) :	SimplePart(src->name, src->params), result(src->result) {}

	OverridePart::OverridePart(Type *parent, Function *src) :
		SimplePart(src->name, new (src) Array<Value>(src->params)),
		result(src->result) {

		params->at(0) = thisPtr(parent);
	}

	Int OverridePart::matches(Named *candidate) const {
		Function *fn = as<Function>(candidate);
		if (!fn)
			return -1;

		Array<Value> *c = fn->params;
		if (c->count() != params->count())
			return -1;

		if (c->count() < 1)
			return -1;

		if (params->at(0).canStore(c->at(0))) {
			// Candidate is in a subclass wrt us.
			for (nat i = 1; i < c->count(); i++) {
				// Candidate needs to accept wider inputs than us.
				if (!c->at(i).canStore(params->at(i)))
					return -1;
			}

			// Candidate may return a narrower range of types.
			if (!fn->result.canStore(result))
				return -1;

		} else if (c->at(0).canStore(params->at(0))) {
			// Candidate is in a superclass wrt us.
			for (nat i = 1; i < c->count(); i++) {
				// We need to accept wider inputs than candidate.
				if (!params->at(i).canStore(c->at(i)))
					return -1;
			}

			// We may return a narrower range than candidate.
			if (!result.canStore(fn->result))
				return -1;

		} else {
			return -1;
		}

		// We always give a binary decision.
		return 0;
	}

}
