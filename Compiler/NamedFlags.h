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

	};

	BITMASK_OPERATORS(NamedFlags);

}
