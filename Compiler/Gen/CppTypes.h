#pragma once

namespace storm {

	/**
	 * Describes the types declared in C++ which are garbage collected and exposed to Storm.
	 */

	/**
	 * List of C++ types.
	 */
	struct CppType {
		// Name of the type (null if last element).
		const wchar *name;

		// Parent class' type id.
		nat parent;

		// Total size of the type.
		size_t size;

		// Invalid pointer offset.
		static size_t invalidOffset = -1;

		// Pointer offsets. Ends with 'invalid_offset'.
		size_t *ptrOffsets;
	};


	/**
	 * Contains all information about C++ types and functions required by Storm.
	 */
	struct CppWorld {
		// List of types.
		CppType *types;
	};

	// Get the CppWorld for this module.
	const CppWorld *cppWorld();

}
