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
	 * Create a ValueRef.
	 */
	ValueRef valueRef(const wchar *name, bool ref) {
		ValueRef r = { name, ref ? ValueRef::ref : ValueRef::nothing };
		return r;
	}

	/**
	 * Null value.
	 */
	ValueRef valueRef() {
		ValueRef r = { null, ValueRef::nothing };
		return r;
	}

	/**
	 * Create a valueref pointing to an array of type "t".
	 */
	ValueRef arrayRef(const wchar *name, bool ref) {
		ValueRef r = valueRef(name, ref);
		r.options = r.options | ValueRef::array;
		return r;
	}

	/**
	 * Wrap the default generated assignment operator. Since the calling convention differ,
	 * we need to wrap it like this.
	 */
	template <class T>
	T &CODECALL wrapAssign(T &to, const T &from) {
		return to = from;
	}

	/**
	 * Create a vector from a argument list.
	 */
	vector<ValueRef> list(nat count, ...) {
		va_list l;
		va_start(l, count);

		vector<ValueRef> result;
		for (nat i = 0; i < count; i++)
			result.push_back(va_arg(l, ValueRef));

		va_end(l);
		return result;
	}

	/**
	 * Constructor for built-in classes.
	 */
	template <class T>
	void create1(void *mem) {
		new (mem)T();
	}

	template <class T, class P>
	void create2(void *mem, P p) {
		new (mem)T(p);
	}

	template <class T, class P, class Q>
	void create3(void *mem, P p, Q q) {
		new (mem)T(p, q);
	}

	template <class T, class P, class Q, class R>
	void create4(void *mem, P p, Q q, R r) {
		new (mem)T(p, q, r);
	}

	template <class T, class P, class Q, class R, class S>
	void create5(void *mem, P p, Q q, R r, S s) {
		new (mem)T(p, q, r, s);
	}

	template <class T, class P, class Q, class R, class S, class U>
	void create6(void *mem, P p, Q q, R r, S s, U u) {
		new (mem)T(p, q, r, s, u);
	}

	/**
	 * Destructor for built-in value types.
	 */
	template <class T>
	void destroy(const T &v) {
		v.~T();
	}

	/**
	 * Everything between BEGIN TYPES and END TYPES is auto-generated.
	 */
	const BuiltInType *builtInTypes() {
		static BuiltInType types[] = {
			// BEGIN TYPES
			// END TYPES
			{ null, null, 0, 0, 0, 0, null },
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
			{ null, null, valueRef(), null, list(0), null },
		};
		return fns;
	}

	/**
	 * Everything between BEGIN THREADS and END THREADS is auto-generated.
	 */
	const BuiltInThread *builtInThreads() {
		static BuiltInThread threads[] = {
			// BEGIN THREADS
			// END THREADS
			{ null, null, null },
		};
		return threads;
	}

}

#endif
