#pragma once
#include "Core/Str.h"
#include "SrcPos.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Various wrappers used until we have a better solution in Storm.
	 */

	// Throw a syntax error.
	void STORM_FN throwSyntaxError(SrcPos pos, Str *msg);

}
