#pragma once

namespace storm {

	/**
	 * Implementation of operations shared between the number type implementations.
	 */


	template <class T>
	inline T CODECALL numMin(T a, T b) {
		return min(a, b);
	}

	template <class T>
	inline T CODECALL numMax(T a, T b) {
		return max(a, b);
	}

}
