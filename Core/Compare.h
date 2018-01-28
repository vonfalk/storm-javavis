#pragma once

namespace storm {

	/**
     * To minimize the work required for implementing operators, this header implements some
     * convenience overloads. In the end, only either == and < is needed.
	 *
	 * This convention is followed by Storm as well. Storm also generates == if only < is provided.
	 */

	template <class T>
	inline bool operator >(const T &a, const T &b) {
		return b < a;
	}

	template <class T>
	inline bool operator <=(const T &a, const T &b) {
		return !(a > b);
	}

	template <class T>
	inline bool operator >=(const T &a, const T &b) {
		return !(a < b);
	}

	template <class T>
	inline bool operator !=(const T &a, const T &b) {
		return !(a == b);
	}

}
