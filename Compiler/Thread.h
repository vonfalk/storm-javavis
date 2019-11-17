#pragma once
#include "Core/Thread.h"
#include "Core/TObject.h"

namespace storm {
	STORM_PKG(core);

	/**
	 * Expose some threading interfaces to Storm.
	 */

	// Get a unique identifier for the current UThread. This identifier is only valid until the
	// current UThread terminates.
	Word STORM_FN currentUThread();

}
