#include "stdafx.h"
// Protect agains accidental compilation from the Shared project.
#ifndef STORM_LIB
#include "Utils/Memory.h"
#include "Shared/BuiltIn.h"
#include "Shared/TypeFlags.h"
// INCLUDE ENGINE
#include <stdarg.h>


// Below are auto-generated includes.
// BEGIN INCLUDES
// END INCLUDES

// Turn off optimizations in this file. It takes quite a long time, and since it is only executed
// during compiler startup, it is not very useful to optimize these functions, especially not during
// debug builds.
#pragma optimize ("", off)

// BEGIN STATIC
// END STATIC

namespace storm {

	/**
	 * Null value.
	 */
	ValueRef valueRef() {
		ValueRef r;
		zeroMem(r);
		return r;
	}

	/**
	 * Create a ValueRef.
	 */
	ValueRef valueRef(const wchar *name, bool ref) {
		ValueRef r = valueRef();
		r.name = name;
		r.options = ref ? ValueRef::ref : ValueRef::nothing;
		return r;
	}

	/**
	 * Create a valueref pointing to an array of type "t".
	 */
	ValueRef arrayRef(ValueRef r) {
		r.options |= ValueRef::array;
		return r;
	}

	/**
	 * Create a valueref indicating a MAYBE on the outermost level.
	 */
	ValueRef maybeRef(ValueRef r) {
		r.options |= ValueRef::maybe;
		return r;
	}

	/**
	 * Make sure a ValueRef is not a function pointer.
	 */
	void ensureInner(const ValueRef &v) {
		assert((v.options & ValueRef::fnPtr) == 0, L"Recursive function pointer definitions not supported!");
	}

	/**
	 * Create a valueref for a function pointer.
	 */
	ValueRef ptrRef(ValueRef result) {
		ensureInner(result);
		ValueRef r = valueRef();
		r.name = L"";
		r.options |= ValueRef::fnPtr;
		r.result = result;
		return r;
	}

	ValueRef ptrRef(ValueRef result, ValueRef p1) {
		ensureInner(p1);
		ValueRef r = ptrRef(result);
		r.params[0] = p1;
		return r;
	}

	ValueRef ptrRef(ValueRef result, ValueRef p1, ValueRef p2) {
		ensureInner(p2);
		ValueRef r = ptrRef(result, p1);
		r.params[1] = p2;
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

	template <class T, class P, class Q, class R, class S, class U, class V>
	void create7(void *mem, P p, Q q, R r, S s, U u, V v) {
		new (mem)T(p, q, r, s, u, v);
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
			{ null, null, 0, 0, BuiltInType::superNone, 0, 0, null },
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
			{ BuiltInFunction::Mode(0), null, 0, valueRef(), null, list(0), null, null },
		};
		return fns;
	}

	/**
	 * Everything between BEGIN VARS and END VARS is auto-generated.
	 */
	const BuiltInVar *builtInVars() {
		static BuiltInVar vars[] = {
			// BEGIN VARS
			// END VARS
			{ 0, valueRef(), null, 0 },
		};
		return vars;
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

	/**
	 * All built in stuff.
	 */
	const BuiltIn &builtIn() {
		static BuiltIn stuff = {
			builtInTypes(),
			builtInFunctions(),
			builtInVars(),
			builtInThreads(),
			cppVTable_storm__Object(),
		};
		return stuff;
	}

}

// Return optimizations to normal.
#pragma optimize ("", on)

#endif
