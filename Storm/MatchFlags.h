#pragma once
#include "Utils/Bitmask.h"

namespace storm {

	// Modify the behaviour of the lookup process slightly for this function.
	enum MatchFlags {
		// No flags, default matching.
		matchDefault = 0x0,

		// Ignore inheritance when matching types for this Named.
		matchNoInheritance = 0x1,

		// This function is suitable for automatic casts (only constructors).
		matchAutoCast = 0x2,
	};

	BITMASK_OPERATORS(MatchFlags);

}
