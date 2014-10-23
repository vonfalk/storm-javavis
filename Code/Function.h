#pragma once

namespace code {

	/**
	 * Helpers to call functions in more flexible ways than what
	 * is allowed by 'regular' c++. These are implemented in a machine-specific way.
	 */

	void *fnCall(void *fnPtr, nat paramCount, const void **params);

	// Call a function taking an arbitrary number of pointer arguments. Assumed to return a pointer.
	template <class R, class T>
	R *fnCall(void *fnPtr, const vector<T *> &params) {
		void *r = fnCall(fnPtr, params.size(), (const void **)&params[0]);
		return (R*)r;
	}

}
