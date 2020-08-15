#pragma once
#include "NameSet.h"
#include "ReplaceContext.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Utility functions and types for dealing with templates generated while loading or reloading
	 * some piece of code.
	 */

	// Remove all templates that depend on something inside 'root'.
	void removeTemplatesFrom(NameSet *root);

	// Find templates that depend on something inside 'root' so that they may be merged at a later
	// point. Uses type equivalences in 'context', and updates 'context' with new equivalences.
	Array<NamedPair> *replaceTemplatesFrom(NameSet *root, ReplaceContext *context);

}
