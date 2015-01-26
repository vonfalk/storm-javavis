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
	}

	void ptrace(Int z) {
		PLN("At: " << z);
	}
}
