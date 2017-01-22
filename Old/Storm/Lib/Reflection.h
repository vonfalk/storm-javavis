#pragma once
#include "Std.h"
#include "Shared/TObject.h"

namespace storm {
	STORM_PKG(core.lang);

	class Type;

	/**
	 * Functions for accessing type information and reflection features from Storm itself. This part
	 * of the library could be omitted when doing a release, and should therefore be used with care.
	 */

	// Get the type of an object. Value types are not supported!
	Type *STORM_FN typeOf(Par<Object> object);
	Type *STORM_FN typeOf(Par<TObject> object);

}
