#pragma once
#include "Name.h"

namespace storm {

	/**
	 * A list of all built-in functions.
	 */
	struct BuiltInFunction {

		// The location (package or typename).
		Name pkg;

		// Name of the return type.
		Name result;

		// Name of the function.
		String name;

		// Parameters to the function (Name of the types).
		vector<Name> params;

		// Function pointer. Null if the last element.
		void *fnPtr;
	};

	/**
	 * Get the list. The list ends with a function with a null 'fnPtr'.
	 */
	const BuiltInFunction *builtInFunctions();

}
