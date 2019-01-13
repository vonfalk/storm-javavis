#pragma once
#include "Utils/Bitmask.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Define different properties for a type.
	 */
	enum TypeFlags {
		// No info.
		typeNone = 0x00,

		// Regular type.
		typeClass = 0x01,

		// Value type.
		typeValue = 0x02,

		// This type is final.
		typeFinal = 0x10,

		// More to come!

		// This is a type that comes from C++. This means that Storm will not attempt to create a
		// layout for the object automatically.
		typeCpp = 0x1000,

		// This type comes from C++ and is a POD type.
		typeCppPOD = 0x2000,

		// This type comes from C++ and is a simple type (ie. trivial copy ctor and dtor).
		typeCppSimple = 0x4000,
	};

	BITMASK_OPERATORS(TypeFlags);
}
