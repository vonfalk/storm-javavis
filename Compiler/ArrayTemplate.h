#pragma once
#include "ValueArray.h"
#include "Type.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Implements the template interface for the Array<> class in Storm.
	 */

	// Create types for unknown implementations.
	Type *createArray(ValueArray *params);

	// Type for arrays.
	class ArrayType : public Type {
		STORM_CLASS;
	public:
	};

}
