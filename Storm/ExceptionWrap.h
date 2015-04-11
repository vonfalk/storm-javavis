#pragma once
#include "SrcPos.h"
#include "Lib/Str.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * (temporary) wrappers for throwing exceptions in Storm.
	 */
	void STORM_FN throwSyntaxError(SrcPos pos, Par<Str> msg);

}
