#pragma once
#include "Utils/Bitmask.h"

namespace storm {

	/**
	 * Define different properties for a type.
	 */
	enum TypeFlags {
		// Regular type.
		typeClass = 0x01,

		// Value type.
		typeValue = 0x02,

		// More to come!
	};

	BITMASK_OPERATORS(TypeFlags);
}
