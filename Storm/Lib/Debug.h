#pragma once
#include "Object.h"
#include "Int.h"

namespace storm {
	STORM_PKG(core.debug);

	/**
	 * Debug functions useful while developing the compiler itself. May or may
	 * not be available in release mode later on. TODO: decide.
	 */

	void STORM_FN dbgBreak();

	// Print the VTable of an object.
	void STORM_FN printVTable(Object *obj);

	// Basic print tracing (to be removed when we have something real).
	void STORM_FN ptrace(Int z);


	// Class partly implemented in C++, we'll try to override this in Storm.
	class Dbg : public Object {
		STORM_CLASS;
	public:
		STORM_CTOR Dbg();

		// Print our state.
		void STORM_FN dbg();

		// Set a value.
		void STORM_FN set(Int v);

		// Get a value.
		virtual Int STORM_FN get();

	private:
		Int v;
	};
}
