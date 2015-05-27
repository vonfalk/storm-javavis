#pragma once
#include "Utils/Bitmask.h"

namespace storm {

	/**
	 * Misc flags for Named objects.
	 */
	enum NamedFlags {
		// No flags, default behavior.
		namedDefault = 0x00,


		/**
		 * Flags for function matching:
		 */

		// Ignore inheritance when matching types for this Named.
		namedMatchNoInheritance = 0x01,

		// This function is suitable for automatic casts (only constructors).
		namedMatchAutoCast = 0x02,

		/**
		 * Overloading. (mostly useful for functions)
		 * TODO: Add member meaning: expected to overload something.
		 */

		// This named is final.
		namedFinal = 0x10,

		/**
		 * Access.
		 * TODO: Implement private, package, protected and public in some way.
		 */

	};

	BITMASK_OPERATORS(NamedFlags);

}
