#pragma once

#include "Core/Io/Url.h"
#include "Core/Array.h"

namespace storm {
	STORM_PKG(core.sys);

	// Open an URL using the program associated with it in the system.
	void STORM_FN open(Url *file);

	// Execute a program (possibly in Path).
	void STORM_FN execute(Url *program, Array<Str *> *params);
}
