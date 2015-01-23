#include "stdafx.h"
#include "VTable.h"
#include "Engine.h"
#include "Code/VTable.h"
#include "Code/Binary.h"

namespace storm {

	wostream &operator <<(wostream &to, const VTablePos &pos) {
		to << pos.stormEntry ? L"storm" : L"c++";
		to << L":" << pos.offset;
		return to;
	}

	VTable::VTable(Engine &e) : ref(e.arena, L"vtable"), engine(e), cppVTable(null), replaced(null) {}

	VTable::~VTable() {
		// TODO: Delay!
		delete replaced;
	}

	void VTable::create(void *cppVTable) {
		assert(replaced == null && this->cppVTable == null);
		this->cppVTable = cppVTable;
		ref.set(cppVTable);
	}

	void VTable::create(const VTable &parent) {
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

		// Reduced prolog.
		l << mov(ptrFrame, ptrStack);

		// Compute address!
		l << mov(ptrA, v);
		// Get the C++ vtable
		l << mov(ptrA, ptrRel(ptrA));
		// Get the Storm vtable
		l << mov(ptrA, ptrRel(ptrA, Offset::sPtr * -2));
		// Get the actual entry.
		l << mov(ptrA, ptrRel(ptrA, Offset::sPtr * i));
		// Call it! (size does not matter in this case).
		l << code::call(ptrA, Size());

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
		l << mov(ptrFrame, ptrStack);

		// Compute address!
		l << mov(ptrA, v);
		// Get the C++ vtable
		l << mov(ptrA, ptrRel(ptrA));
		// Get the actual entry.
		l << mov(ptrA, ptrRel(ptrA, Offset::sPtr * i));
		// Call it!
		l << code::call(ptrA, Size());

		Binary *b = new Binary(engine.arena, L"cppVtableCall" + toS(i), l);
		binaries.push_back(b);

		RefSource *s = new RefSource(engine.arena, L"cppVtableCall" + toS(i));
		b->update(*s);
		return s;
	}

}

