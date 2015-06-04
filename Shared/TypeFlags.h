#pragma once
#include "Utils/Bitmask.h"

namespace storm {

	/**
	 * Define different properties for a type.
	 */
	enum TypeFlags {
		// Regular type.
		typeClass = 0x01,

		// Is it a value type (does not inherit from Object).
		typeValue = 0x02,

		// Final (not possible to override)?
		typeFinal = 0x10,

		// Used together with the class type, and indicates that this type is a raw pointer of some kind.
		// This means several things: no vtables will be used (just like values), reference counting is
		// assumed to be in place, and constructors are assumed to take a void ** as the first parameter,
		// instead of void * (as values do).
		// Used to implement Maybe<T>, and other special pointer types. Use with typeClass.
		typeRawPtr = 0x20,

		// Do not setup inheritance automatically (cleared in the constructor). If set,
		// you are required to manually set classes to inherit from Object.
		typeManualSuper = 0x80,
	};

	BITMASK_OPERATORS(TypeFlags);

}
