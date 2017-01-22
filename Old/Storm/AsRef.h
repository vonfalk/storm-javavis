#pragma once
#include "Function.h"

namespace storm {

	/**
	 * Creates a function that wraps another function, but the parameter(s) are by reference instead
	 * of by value. This is useful when creating functions that has to match C++ function
	 * declarations exactly, eg when using Handle.
	 *
	 * The following conversions are made on all parameters:
	 * value -> value ref
	 * value ref -> value ref (no change)
	 * class -> class ref
	 * class ref -> class ref (no change)
	 */
	class AsRef : NoCopy {
	public:
		// Create.
		AsRef(code::Arena &arena, const Value &result, const ValList &params, const code::Ref &fn, bool isMember);

		// Create from function.
		AsRef(Par<Function> from);

		// Reference to the wrapper code.
		code::RefSource source;
	};

}
