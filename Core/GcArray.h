#pragma once

namespace storm {

	/**
	 * Helper struct to access gc:d arrays. The preprocessor recognices this class, so it is
	 * properly marked for the gc.
	 */
	template <class T>
	struct GcArray {
		// Number of elements. Set on allocation and then not allowed to change.
		const size_t count;

		// Number of used elements. Ignored by the GC and its usage is up to the user of the array.
		size_t filled;

		// Data.
		T v[1];
	};

}
