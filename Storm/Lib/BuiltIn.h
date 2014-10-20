#pragma once
#include "Name.h"

namespace storm {

	/**
	 * A list of all built-in classes.
	 */
	struct BuiltInType {

		// Location (package).
		Name pkg;

		// Name of the type (null in the last element).
		const wchar *name;
	};

	/**
	 * A list of all built-in functions.
	 */
	struct BuiltInFunction {

		// The location (package).
		Name pkg;

		// Member of a type? null=no.
		const wchar *typeMember;

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
	 * Get list of built-in types.
	 */
	const BuiltInType *builtInTypes();

	/**
	 * Get the list. The list ends with a function with a null 'fnPtr'.
	 */
	const BuiltInFunction *builtInFunctions();

}
