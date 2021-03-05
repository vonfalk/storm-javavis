#pragma once

#include "Core/Io/Url.h"

namespace storm {
	STORM_PKG(core.sys);

	// Open an URL using the program associated with it in the system.
	void STORM_FN open(Url *file);

	// TODO: Execute a program with the specified parameters.
}
