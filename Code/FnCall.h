#pragma once
#include "FnParams.h"
#include "Utils/Templates.h"

namespace code {

	/**
	 * Helpers for calling functions in various non-standard ways.
	 * Useful in conjunction with the FnParams header.
	 */

	// Call a function with the given parameters. Places the result in 'result', assuming
	// the type described by the BasicTypeInfo. If 'memberFn' is true, 'fn' is a member of
	// a class (this affects how results are returned sometimes).
	void call(const void *fn, bool memberFn, const FnParams &params, void *result, const BasicTypeInfo &info);

	// Call a function with pre-allocated parameters. These are assumed to be somewhere
	// higher up on the stack. Places the result in 'result', assuming the type described
	// by the BasicTypeInfo.
	void call(const void *fn, bool memberFn, void *params, void *result, const BasicTypeInfo &info);


	// Regular function call. Does not handle references, use callRef in that case.
	template <class T>
	inline T call(const void *fn, bool memberFn, const FnParams &params = FnParams()) {
		byte d[sizeof(T)];
		call(fn, memberFn, params, d, typeInfo<T>());
		T *result = (T *)d;
		T copy = *result;
		result->~T();
		return copy;
	}

	// Specific void implementation
	template <>
	inline void call(const void *fn, bool memberFn, const FnParams &params) {
		code::call(fn, memberFn, params, null, typeInfo<void>());
	}

	// Reference function call.
	template <class T>
	inline T &callRef(const void *fn, bool memberFn, const FnParams &params = FnParams()) {
		T *result = null;
		call(fn, memberFn, params, &result, typeInfo<T &>());
		return *result;
	}

}
