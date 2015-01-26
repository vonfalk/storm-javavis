#include "stdafx.h"
#include "VTable.h"
#include "Engine.h"
#include "Code/VTable.h"
#include "Code/Binary.h"
#include "Function.h"
#include <iomanip>

namespace storm {

	VTable::VTable(Engine &e)
		: ref(e.arena, L"vtable"), engine(e), cppVTable(null),
		  storm(replaced), replaced(null), parent(null) {}

	VTable::~VTable() {
		// TODO: Delay! Note, also delay the destruction of our StormVTable! Might be easier to delay
		// the destruction of the entire owning Type class.
		del(replaced);
	}

	void VTable::create(void *cppVTable) {
		assert(replaced == null && this->cppVTable == null);
		this->cppVTable = cppVTable;
		ref.set(cppVTable);

		// Classes created from a VTable does not need parent/child information.
	}

	void VTable::create(VTable &parent) {
		cppVTable = parent.cppVTable;
		if (!replaced) {
			if (parent.replaced)
				replaced = new code::VTable(*parent.replaced);
			else
				replaced = new code::VTable(cppVTable, engine.maxCppVTable());
		} else {
			if (parent.replaced)
				replaced->replace(*parent.replaced);
			else
				replaced->replace(cppVTable);
		}

		ref.set(replaced->ptr());
		storm.clear();

		// Update child/parent relation.
		if (this->parent)
			this->parent->children.erase(this);
		this->parent = &parent;
		parent.children.insert(this);

		// Clear all our children.
		for (ChildSet::iterator i = children.begin(); i != children.end(); ++i) {
			VTable *c = *i;
			c->create(*this);
		}
	}

	void VTable::create() {
		// TODO: Implement
		assert(("Creating an empty VTable is not supported yet!", false));
		if (parent)
			parent->children.erase(this);
		parent = null;
	}

	void VTable::update(Object *object) {
		if (replaced)
			replaced->setTo(object);
		else if (cppVTable)
			code::setVTable(object, cppVTable);
	}

	bool VTable::builtIn() const {
		return replaced == null && cppVTable != null;
	}

	VTablePos VTable::insert(Function *fn) {
		// Check if this one has been inserted before, as a super-type.
		nat slot = findSlot(fn);

		// May have been inserted before.
		if (storm.item(slot) == null) {
			VTableUpdater *updater = new VTableUpdater(*this, fn);
			storm.item(slot, updater);
			updater->update();
		}

		return VTablePos(slot, true);
	}

	bool VTable::inserted(Function *fn) {
		for (nat i = 0; i < storm.count(); i++) {
			VTableUpdater *item = storm.item(i);
			if (item != null && item->fn == fn)
				return true;
		}
		return false;
	}

	void VTable::updateAddr(nat id, void *to, VTableUpdater *src) {
		// We've got our own implementation!
		if (storm.item(id) != null && storm.item(id) != src)
			return;

		storm.addr(id, to);

		for (ChildSet::iterator i = children.begin(); i != children.end(); ++i)
			(*i)->updateAddr(id, to, src);
	}

	nat VTable::findSlot(Function *fn) {
		nat slot = storm.findItem(fn);
		if (slot < storm.count())
			return slot;

		// If 'fn' is an overload, we need to reuse the previous slot.
		slot = findBase(fn);
		if (slot < storm.count())
			return slot;

		// Find an unused slot. Do not use any from the parents range.
		if (parent)
			slot = storm.emptyItem(parent->storm.count());
		else
			slot = storm.emptyItem();

		if (slot < storm.count())
			return slot;

		// Allocate a new slot.
		slot = storm.count();
		expand(slot, 1);
		return slot;
	}

	nat VTable::findBase(Function *fn) {
		// TODO: More efficient?
		for (nat i = 0; i < storm.count(); i++) {
			VTableUpdater *item = storm.item(i);
			if (item != null && isOverload(item->fn, fn))
				return i;
		}

		if (parent)
			return parent->findBase(fn);
		return storm.count();
	}

	void VTable::expand(nat at, nat insert) {
		storm.expand(at, insert);

		for (ChildSet::iterator i = children.begin(); i != children.end(); ++i)
			(*i)->contract(at, insert);
	}

	void VTable::contract(nat from, nat to) {
		storm.expand(from, to);

		for (ChildSet::iterator i = children.begin(); i != children.end(); ++i)
			(*i)->contract(from, to);
	}

	void VTable::dbg_dump() {
		wostream &to = std::wcout;
		to << L"Vtable: ";
		if (replaced)
			to << replaced->ptr() << L", " << replaced->extra();
		else
			to << cppVTable;
		to << endl;

		Indent z(to);
		if (builtIn()) {
			to << L"Built in class, no vtable." << endl;
			return;
		}

		for (nat i = 0; i < storm.count(); i++) {
			to << std::setw(3) << i << L": ";
			to << storm.addr(i) << L" (";

			VTableUpdater *u = storm.item(i);
			if (u)
				to << *u->fn;
			else
				to << L"null";
			to << L")" << endl;
		}

		for (ChildSet::iterator i = children.begin(); i != children.end(); ++i)
			(*i)->dbg_dump();
	}


	/**
	 * VTable code.
	 */


	VTableCalls::VTableCalls(Engine &e) : engine(e) {}

	VTableCalls::~VTableCalls() {
		clear(stormCreated);
		clear(cppCreated);
		clear(binaries);
	}

	code::Ref VTableCalls::call(VTablePos pos) {
		if (pos.stormEntry)
			return call(pos.offset, stormCreated, &VTableCalls::createStorm);
		else
			return call(pos.offset, cppCreated, &VTableCalls::createCpp);
	}

	code::Ref VTableCalls::call(nat i, vector<code::RefSource *> &src, code::RefSource *(VTableCalls::*create)(nat)) {
		if (src.size() <= i)
			src.resize(i + 1, null);

		if (!src[i])
			src[i] = (this->*create)(i);

		return code::Ref(*src[i]);
	}

	code::RefSource *VTableCalls::createStorm(nat i) {
		using namespace code;
		Listing l;
		Variable v = l.frame.createPtrParam();

		// TODO: Make less machine dependent!

		// TODO: Be able to make a naked function, without the need for a prolog and epilog!
		l << prolog();

		// Compute address!
		l << mov(ptrA, v);
		// Get the C++ vtable
		l << mov(ptrA, ptrRel(ptrA));
		// Get the Storm vtable
		l << mov(ptrA, ptrRel(ptrA, Offset::sPtr * -2));
		// Get the actual entry.
		l << mov(ptrA, ptrRel(ptrA, Offset::sPtr * i));

		// Call it!
		l << epilog();
		l << jmp(ptrA);

		Binary *b = new Binary(engine.arena, L"stormVtableCall" + toS(i), l);
		binaries.push_back(b);

		RefSource *s = new RefSource(engine.arena, L"stormVtableCall" + toS(i));
		b->update(*s);
		return s;
	}

	code::RefSource *VTableCalls::createCpp(nat i) {
		using namespace code;
		Listing l;
		Variable v = l.frame.createPtrParam();

		// TODO: Make less machine dependent!

		// Reduced prolog.
		l << prolog();

		// Compute address!
		l << mov(ptrA, v);
		// Get the C++ vtable
		l << mov(ptrA, ptrRel(ptrA));
		// Get the actual entry.
		l << mov(ptrA, ptrRel(ptrA, Offset::sPtr * i));

		// Call it!
		l << epilog();
		l << jmp(ptrA);

		Binary *b = new Binary(engine.arena, L"cppVtableCall" + toS(i), l);
		binaries.push_back(b);

		RefSource *s = new RefSource(engine.arena, L"cppVtableCall" + toS(i));
		b->update(*s);
		return s;
	}

}

