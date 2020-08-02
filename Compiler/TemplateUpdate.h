#pragma once
#include "NameSet.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Utility functions and types for dealing with templates generated while loading or reloading
	 * some piece of code.
	 */

	// Remove all templates that depend on something inside 'root'.
	void removeTemplatesFrom(NameSet *root);

}
