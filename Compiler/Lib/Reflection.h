#pragma once
#include "Core/Object.h"
#include "Core/TObject.h"

namespace storm {
	namespace reflect {
		STORM_PKG(core.lang);

		/**
		 * Storm-facing reflection API. From C++, prefer what is inside of the runtime:: namespace
		 * (see Core/Runtime.h).
		 */

		// Get the run-time type of a heap-allocated object.
		Type *STORM_FN typeOf(Object *o);
		Type *STORM_FN typeOf(TObject *o);

	}
}
