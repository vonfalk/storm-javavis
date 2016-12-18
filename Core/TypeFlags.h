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

		// Used together with the class type and indicates that this type is a raw pointer of some
		// kind. This means several things: no vtables will be used (just like values), and
		// constructors are assumed to take a void ** as their first parameter instead of void * (as
		// values do). Used to implement Maybe<T> and other special pointer types. Use with typeClass.
		typeRawPtr = 0x20,

		// More to come!

		// This is a type that comes from C++.
		typeCpp = 0x1000,
	};

	BITMASK_OPERATORS(TypeFlags);
}
