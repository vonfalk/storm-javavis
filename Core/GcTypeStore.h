#pragma once
#include "GcType.h"

namespace storm {

	/**
	 * Type that allows specifying more than one reference inside a GcType.
	 */
	template <size_t elements>
	struct GcTypeStore {
		// The first element.
		GcType type;

		// Remaining elements. This will be properly aligned since both pointers and size_t have the
		// same size.
		size_t offset[elements - 1];
	};

}
