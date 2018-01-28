#pragma once
#include "Function.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Make a small function which wraps another function so that all parameters are received as
	 * references. No changes are made to the return value.
	 */
	const void *makeRefParams(Function *wrap);

	// See if 'makeRefParams' is unnecessary.
	bool allRefParams(Function *fn);

}
