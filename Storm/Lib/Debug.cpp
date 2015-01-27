#include "stdafx.h"
#include "Debug.h"
#include "Code/VTable.h"
#include "Type.h"

namespace storm {

	void dbgBreak() {
		DebugBreak();
	}

	void printVTable(Object *obj) {
		void *v = code::vtableOf(obj);
		PLN("Vtable of: " << obj << " is " << v);
		PLN(" Refs: " << obj->dbg_refs());
	}

	void ptrace(Int z) {
		PLN("At: " << z);
	}


	Dbg::Dbg() : v(10) {}

	void Dbg::set(Int v) {
		this->v = v;
	}

	Int Dbg::get() {
		return v;
	}

	void Dbg::dbg() {
		PLN("Debug object. Type: " << myType->identifier() << " value: " << v);
	}

}
