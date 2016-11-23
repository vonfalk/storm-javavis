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
		// Do we already know this function?
		if (updaters->has(fn))
			return;

		VTableSlot slot = findSlot(fn, true);

		if (!slot.valid()) {
			// We need to allocate a slot for this function.
			slot = allocSlot();
		}

		// Insert into the vtable.
		set(slot, fn);
		updaters->put(fn, new (this) VTableUpdater(this, slot, fn->directRef(), source->content()));

		// See if there are any child functions which we potentially need to update.
		if (updateChildren(fn, slot))
			useLookup(fn, slot);
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


	VTableSlot VTable::findSlot(Function *fn, bool setLookup) {
		Code *c = fn->getCode();
		if (as<StaticCode>(c)) {
			// Might be a function from C++.
			const void *addr = fn->directRef()->address();
			nat r = cpp ? cpp->findSlot(fn) : vtable::find(original, addr);
			if (r != vtable::invalid)
				return cppSlot(r);
		}

		nat r = vtable::invalid;
		if (storm)
			r = storm->findSlot(fn);

		if (r != vtable::invalid)
			return stormSlot(r);

		// See if our parent has a match!
		if (Type *s = owner->super()) {
			return s->vtable->findSuperSlot(new (this) OverridePart(fn), setLookup);
		} else {
			return VTableSlot();
		}
	}

	VTableSlot VTable::findSuperSlot(OverridePart *fn, bool setLookup) {
		Function *found = as<Function>(owner->findHere(fn));
		if (found) {
			// If it is not known to us yet, we can not do much about it.
			if (updaters->has(found)) {
				VTableSlot slot = updaters->get(found)->slot();
				if (setLookup)
					useLookup(found, slot);
				return slot;
			}
		}

		// Try to find it in superclasses.
		if (Type *s = owner->super()) {
			return s->vtable->findSuperSlot(fn, setLookup);
		} else {
			return VTableSlot();
		}
	}

	Bool VTable::updateChildren(Function *fn, VTableSlot slot) {
		if (!owner->chain)
			return false;

		OverridePart *p = new (this) OverridePart(fn);

		Array<Type *> *c = owner->chain->children();
		bool found = false;
		for (nat i = 0; i < c->count(); i++)
			found |= c->at(i)->vtable->updateChildren(p, slot);

		return found;
	}

	Bool VTable::updateChildren(OverridePart *fn, VTableSlot slot) {
		bool found = false;

		if (Function *f = as<Function>(owner->findHere(fn))) {
			// If it is not known to us, we can not do anything useful with it.
			if (VTableUpdater *u = updaters->get(f, null)) {
				// If we do not need to change the slot, we do not need to recurse anymore. No other
				// overriding functions need to be updated.
				if (u->slot() == slot)
					return true;

				// Otherwise, we need to change slot for this function:
				set(slot, f);
				u->slot(slot);
				clear(slot);
			}
		}

		// Go on recursing!
		Array<Type *> *c = owner->chain->children();
		for (nat i = 0; i < c->count(); i++)
			found |= c->at(i)->vtable->updateChildren(fn, slot);
		return found;
	}

	VTableSlot VTable::allocSlot() {
		if (!storm) {
			assert(false, L"Can not allocate slots in a C++ vtable.");
			return VTableSlot();
		}

		nat slot = storm->freeSlot(stormFirst);
		if (slot >= storm->count()) {
			nat oldCount = storm->count();

			// We need to resize 'storm'...
			storm->resize(oldCount + stormGrow);

			// Grow all child vtables as well.
			Array<Type *> *c = owner->chain->children();
			for (nat i = 0; i < c->count(); i++)
				c->at(i)->vtable->parentGrown(oldCount, stormGrow);
		}

		return stormSlot(slot);
	}

	void VTable::parentGrown(Nat pos, Nat count) {
		if (storm)
			storm->insert(pos, count);
		if (pos >= stormFirst)
			stormFirst += count;

		Array<Type *> *c = owner->chain->children();
		for (nat i = 0; i < c->count(); i++) {
			c->at(i)->vtable->parentGrown(pos, count);
		}

		for (Map<Function *, VTableUpdater *>::Iter i = updaters->begin(), e = updaters->end(); i != e; ++i) {
			Function *f = i.k();
			// Using a lookup?
			if (f->ref()->address() == f->directRef()->address())
				continue;

			VTableUpdater *u = i.v();
			VTableSlot slot = u->slot();
			// Relevant to update?
			if (slot.type != VTableSlot::tStorm || slot.offset < pos)
				continue;

			// Update!
			VTableSlot to = slot;
			to.offset += count;
			set(to, f);
			u->slot(to);
			clear(slot);
		}
	}

	void VTable::useLookup(Function *fn, VTableSlot slot) {
		code::RefSource *src = fn->engine().vtableCalls()->get(slot);
		fn->setLookup(new (fn) DelegatedCode(code::Ref(src)));
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

}
