#include "stdafx.h"
#include "VTable.h"
#include "Engine.h"
#include "Code/VTable.h"
#include "Code/Binary.h"
#include "Function.h"
#include "Exception.h"
#include "TypeDtor.h"
#include <iomanip>

namespace storm {

	/**
	 * Redirect the C++ dtor to the storm dtor (using the vtable). Needs to be in a struct
	 * since we have to be able to use __thiscall (not possible otherwise).
	 *
	 * Note that we will never instantiate this struct. In the member function, the 'this' pointer
	 * will not even be of the correct type.
	 */

	VTable::VTable(Engine &e)
		: ref(e.arena, L"vtable"), engine(e), cppVTable(null),
		  storm(replaced), replaced(null), parent(null) {}

	VTable::~VTable() {
		// TODO: Delay! Note, also delay the destruction of our StormVTable! Might be easier to delay
		// the destruction of the entire owning Type class.
		del(replaced);
		clear(cppSlots);
	}

	void VTable::create(void *cppVTable) {
		assert(replaced == null && this->cppVTable == null);
		this->cppVTable = cppVTable;
		ref.set(cppVTable);

		nat cppCount = 0;
		if (cppVTable)
			cppCount = code::vtableCount(cppVTable);
		cppSlots = vector<VTableSlot*>(cppCount, null);

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
		storm.ensure(1); // For the destructor.
		cppSlots = vector<VTableSlot*>(replaced->count(), null);

		// We need to replace the C++ dtor with our stub!
		replaced->setDtor(dtorRedirect());

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
		assert(false, "Creating an empty VTable is not supported yet!");
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

	VTableSlot *VTable::slot(VTablePos pos) {
		switch (pos.type) {
		case VTablePos::tStorm:
			return storm.slot(pos.offset);
		case VTablePos::tCpp:
			return cppSlots[pos.offset];
		}
		assert(false);
		return null;
	}

	void VTable::slot(VTablePos pos, VTableSlot *to) {
		switch (pos.type) {
		case VTablePos::tStorm:
			storm.slot(pos.offset, to);
			break;
		case VTablePos::tCpp:
			cppSlots[pos.offset] = to;
			break;
		default:
			assert(false);
			break;
		}
	}

	void VTable::addr(VTablePos pos, void *to) {
		switch (pos.type) {
		case VTablePos::tStorm:
			storm.addr(pos.offset, to);
			break;
		case VTablePos::tCpp:
			if (replaced)
				replaced->set(pos.offset, to);
			break;
		default:
			assert(false);
			break;
		}
	}

	VTablePos VTable::insert(Function *fn) {
		// Special case: destructors for built in type should not be inserted.
		if (builtIn() && fn->name == Type::DTOR)
			return VTablePos();

		VTablePos r = findSlot(fn);

		// May have been inserted before.
		if (slot(r) == null) {
			VTableSlot *updater = new VTableSlot(*this, fn, r);
			slot(r, updater);
			updater->update();
		}

		return r;
	}

	bool VTable::inserted(Function *fn) {
		for (nat i = 0; i < storm.count(); i++) {
			VTableSlot *slot = storm.slot(i);
			if (slot != null && slot->fn == fn)
				return true;
		}
		return false;
	}

	void VTable::updateAddr(VTablePos id, void *to, VTableSlot *src) {
		VTableSlot *s = slot(id);

		// We've got our own implementation!
		if (s != null && s != src)
			return;

		addr(id, to);

		for (ChildSet::iterator i = children.begin(); i != children.end(); ++i)
			(*i)->updateAddr(id, to, src);
	}

	VTablePos VTable::findSlot(Function *fn) {
		// Destructor? (always in slot 0)
		if (fn->name == Type::DTOR)
			return VTablePos::storm(0);

		// Cpp function?
		if (builtIn()) {
			nat slot = code::findSlot(fn->directRef().address(), cppVTable);
			if (slot == code::VTable::invalid)
				throw InternalError(::toS(*fn) + L" is not properly implemented in C++. "
									L"Failed to find a VTable entry in the C++ vtable for it!");
			return VTablePos::cpp(slot);
		}

		// Inserted here before?
		nat slot = storm.findSlot(fn);
		if (slot < storm.count())
			return VTablePos::storm(slot);

		// If 'fn' is an overload, we need to reuse the previous slot.
		if (VTablePos s = findBase(fn))
			return s;

		// Find an unused slot. Do not use any from the parents range.
		if (parent)
			slot = storm.emptySlot(parent->storm.count());
		else
			slot = storm.emptySlot(1);

		if (slot < storm.count())
			return VTablePos::storm(slot);

		// Allocate a new slot.
		slot = storm.count();
		expand(slot, 1);
		assert(slot >= 1, "Double-allocated the destructor!");
		return VTablePos::storm(slot);
	}

	VTablePos VTable::findBase(Function *fn) {
		// 1: In the C++ vtable?
		VTablePos p = findCppBase(fn);
		if (p)
			return p;

		// 2: In the Storm vtable?
		return findStormBase(fn);
	}

	VTablePos VTable::findCppBase(Function *fn) {
		// TODO: More efficient?
		for (nat i = 0; i < cppSlots.size(); i++) {
			VTableSlot *slot = cppSlots[i];
			if (slot != null && isOverload(slot->fn, fn))
				return VTablePos::cpp(i);
		}

		if (parent)
			return parent->findCppBase(fn);
		return VTablePos();
	}

	VTablePos VTable::findStormBase(Function *fn) {
		// TODO: More efficient?
		for (nat i = 0; i < storm.count(); i++) {
			VTableSlot *slot = storm.slot(i);
			if (slot != null && isOverload(slot->fn, fn))
				return VTablePos::storm(i);
		}

		if (parent)
			return parent->findStormBase(fn);
		return VTablePos();
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

			VTableSlot *u = storm.slot(i);
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
		switch (pos.type) {
		case VTablePos::tStorm:
			return call(pos.offset, stormCreated, &VTableCalls::createStorm);
		case VTablePos::tCpp:
			return call(pos.offset, cppCreated, &VTableCalls::createCpp);
		default:
			assert(false);
			return code::Ref();
		}
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

