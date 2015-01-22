#include "stdafx.h"
#include "VTable.h"
#include "Engine.h"
#include "Code/VTable.h"

namespace storm {

	VTable::VTable(Engine &e) : engine(e), cppVTable(null), replaced(null) {}

	void VTable::create(void *cppVTable) {
		assert(replaced == null && cppVTable == null);
		this->cppVTable = cppVTable;
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

	}

	void VTable::update(Object *object) {
		if (replaced)
			replaced->setTo(object);
		else if (cppVTable)
			code::setVTable(object, cppVTable);
	}

}
