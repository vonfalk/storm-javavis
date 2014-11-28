#include "stdafx.h"
#ifdef IGNORE_THIS_FILE
#include "BuiltIn.h"
#include "Str.h"
#include "Type.h"
#include <stdarg.h>


// Below are auto-generated includes.
// BEGIN INCLUDES
// END INCLUDES

// BEGIN STATIC
// END STATIC

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
		return new (type)T();
	}

	template <class T, class P>
	T *create2(Type *type, P p) {
		return new (type)T(p);
	}

	template <class T, class P, class Q>
	T *create3(Type *type, P p, Q q) {
		return new (type)T(p, q);
	}

	template <class T, class P, class Q, class R>
	T *create4(Type *type, P p, Q q, R r) {
		return new (type)T(p, q, r);
	}

	template <class T, class P, class Q, class R, class S>
	T *create5(Type *type, P p, Q q, R r, S s) {
		return new (type)T(p, q, r, s);
	}

	template <class T, class P, class Q, class R, class S, class U>
	T *create6(Type *type, P p, Q q, R r, S s, U u) {
		return new (type)T(p, q, r, s, u);
	}

	/**
	 * Everything between BEGIN TYPES and END TYPES is auto-generated.
	 */
	const BuiltInType *builtInTypes() {
		static BuiltInType types[] = {
			// BEGIN TYPES
			// END TYPES
			{ L"", null, L"", null },
		};
		return types;
	}

	/**
	 * Everything between BEGIN LIST and END LIST is auto-generated.
	 */
	const BuiltInFunction *builtInFunctions() {
		static BuiltInFunction fns[] = {
			// BEGIN LIST
			// END LIST
			{ Name(), null, Name(), L"", list(0), null },
		};
		return fns;
	}

}

#endif
