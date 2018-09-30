#pragma once
#include "Core/Char.h"

namespace storm {
	STORM_PKG(core.io);

	// Convert a character to/from the ANSI character set (Windows-1252). Returns 0 for unknown characters.
	Byte STORM_FN toANSI(Char ch);
	Char STORM_FN fromANSI(Byte ch);


}
