#pragma once
#include "Utils/Bitmask.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Misc. flags for lookup options.
	 */
	enum NamedFlags {
		// No flags, default behaviour.
		namedDefault = 0x00,

		/**
		 * Flags for function matching.
		 */

		// Ignore inheritance when matching types for this Named.
		namedMatchNoInheritance = 0x01,

		// This function is suitable for automatic casts (only constructors).
		namedAutoCast = 0x02,

		/**
		 * Overloading, mainly for functions.
		 */

		// This named is final.
		namedFinal = 0x10,

		// This named is abstract (ie. has to be overridden).
		namedAbstract = 0x20,

		// This named is expected to override something.
		namedOverride = 0x40,
	};

	BITMASK_OPERATORS(NamedFlags);

}
