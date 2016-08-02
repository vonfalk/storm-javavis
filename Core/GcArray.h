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

		// Data.
		T v[1];
	};

	/**
	 * Helper struct to access gc:d arrays with an additional size_t in their header.
	 */
	template <class T>
	struct GcDynArray {
		// Number of elements. Set on allocation and then not allowed to change.
		const size_t capacity;

		// Number of filled elements. Ignored by the GC and may therefore change without restrictions.
		size_t filled;

		// Data.
		T v[1];
	};

}
