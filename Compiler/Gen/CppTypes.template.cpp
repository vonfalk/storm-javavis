#include "stdafx.h"
#include "CppTypes.h"

// INCLUDES

/**
 * Template for the generated cpp file.
 */
namespace storm {

	static CppType *cppTypes() {
		// PTR_OFFSETS

		static CppType types[] = {
			// CPP_TYPES
			{ null, 0, 0, null },
		};
		return types;
	}

	const CppWorld *cppWorld() {
		static CppWorld w = {
			cppTypes(),
		};
		return &w;
	}

}
