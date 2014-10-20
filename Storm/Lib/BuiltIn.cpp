#include "stdafx.h"
#include "BuiltIn.h"
#include "Str.h"
#include <stdarg.h>

// Below are auto-generated includes.
// BEGIN INCLUDES
#include "Lib/Str.h"
// END INCLUDES

namespace storm {

	/**
	 * Create a vector from a argument list.
	 */
	vector<Name> list(nat count, ...) {
		va_list l;
		va_start(l, count);

		vector<Name> result;
		for (nat i = 0; i < count; i++)
			result.push_back(va_arg(l, Name));

		va_end(l);
		return result;
	}

	/**
	 * Constructor for built-in classes.
	 */
	template <class T>
	T *create1(Type *type) {
		return new T(type);
	}

	template <class T, class P>
	T *create2(Type *type, const P &p) {
		return new T(type, p);
	}

	/**
	 * Everything between BEGIN TYPES and END TYPES is auto-generated.
	 */
	const BuiltInType *builtInTypes() {
		static BuiltInType types[] = {
			// BEGIN TYPES
			{ Name(L"core"), L"Str", null },
			// END TYPES
			{ L"", null },
		};
		return types;
	}

	/**
	 * Everything between BEGIN LIST and END LIST is auto-generated.
	 */
	const BuiltInFunction *builtInFunctions() {
		static BuiltInFunction fns[] = {
			// BEGIN LIST
			{ Name(L"core"), L"Str", Name(L"Str"), L"__ctor", list(1, Name(L"Type")), address(&create1<storm::Str>) },
			{ Name(L"core"), L"Str", Name(L"Str"), L"__ctor", list(2, Name(L"Type"), Name(L"Str")), address(&create2<storm::Str, Str>) },
			{ Name(L"core"), L"Str", Name(L"Nat"), L"count", list(0), address(&storm::Str::count) },
			// END LIST
			{ Name(), null, Name(), L"", list(0), null },
		};
		return fns;
	}

}