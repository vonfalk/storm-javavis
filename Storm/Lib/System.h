#pragma once
#include "Object.h"
#include "Int.h"

namespace storm {
	STORM_PKG(core.debug);

	/**
	 * System functions useful while developing the compiler itself. May or may
	 * not be available in release mode later on. TODO: decide.
	 */

	void STORM_FN dbgBreak();

	// Print the VTable of an object.
	void STORM_FN printVTable(Object *obj);

	// Basic print tracing (to be removed when we have something real).
	void STORM_FN ptrace(Int z);

}
