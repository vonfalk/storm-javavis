#pragma once
#include "Object.h"
#include "TObject.h"
#include "CloneEnv.h"
#include "Utils/Templates.h"

namespace storm {
	STORM_PKG(core);

	/**
	 * Function pointers for storm and C++.
	 *
	 * A function pointer here may also contain a this-pointer as well, so that member functions
	 * appear like a regular free function without the first this-parameter.
	 *
	 * Function pointers have slightly different calls semantics compared to regular function calls
	 * in Storm; for function pointers, it is determined entirely at runtime if the parameters are
	 * copied or not. This is because we do not have the same amount of information when dealing
	 * with function pointers.
	 *
	 * Function pointers to members of values are not yet supported.
	 *
	 * TODO: Implement equality check.
	 */

	/**
	 * Target of a function call. Implemented as an abstract class here, so we do not need to expose
	 * Ref and RefSource to everyone.
	 *
	 * These are too low-level to be exposed to Storm.
	 */
	class FnTarget {
	public:
		// Clone this target to somewhere else.
		virtual void cloneTo(void *to, size_t size) const = 0;

		// Get the pointer we're pointing to.
		virtual const void *ptr() const = 0;

		// Add some kind of label to a StrBuf for the string representation.
		virtual void toS(StrBuf *to) const = 0;
	};

	/**
	 * Target for raw function pointers.
	 */
	class RawFnTarget {
	public:
		RawFnTarget(const void *ptr);

		virtual void cloneTo(void *to, size_t size) const;
		virtual const void *ptr() const;
		virtual void toS(StrBuf *to) const;
	private:
		const void *data;
	};

	/**
	 * Base class for a function pointer.
	 */
	class FnBase : public Object {
		STORM_CLASS;
	public:
		// Create from C++.
		FnBase(const void *fn, const RootObject *thisPtr, Bool member, Thread *thread);

		// Create with a generic target and all information given.
		FnBase(const FnTarget &target, const RootObject *thisPtr, Bool member, Thread *thread);

		// Copy.
		STORM_CTOR FnBase(FnBase *o);

		// Deep copy.
		virtual void STORM_FN deepCopy(CloneEnv *env);

		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;

		/**
		 * Call this function:
		 */

		// Call our function (low-level, not typesafe).
		// Takes care of delegating to another thread if needed and adds a this pointer if needed.
		// The 'first' parameter is the first parameter as a TObject (if the first parameter is a TObject)
		// and is used to decide if a thread call is to be made or not.
		// Note: Does *not* copy parameters if needed, but handles the return value properly.
		// Note: if 'params' are statically allocated, make sure it has room for at least one more element!
		template <class R>
		R callRaw(os::FnParams &params, const TObject *first, CloneEnv *env) const {
			byte d[sizeof(R)];
			callRaw(d, typeInfo<R>(), params, first, env);
			R *result = (R *)d;
			R copy = *result;
			result->~R();

			if (needsCopy(first)) {
				if (!env)
					env = new (this) CloneEnv();
				cloned(copy, env);
			}
			return copy;
		}

		template <>
		void callRaw(os::FnParams &params, const TObject *first, CloneEnv *env) const {
			callRaw(null, typeInfo<void>(), params, first, env);
		}

		// Call function with a pointer to the return value.
		void callRaw(void *output, const BasicTypeInfo &type, os::FnParams &params, const TObject *first, CloneEnv *env) const;

		// Do we need to copy the parameters for this function given the first TObject?
		bool needsCopy(const TObject *first) const;

	public:
		// Are we calling a member function?
		Bool callMember;

		// This pointer.
		UNKNOWN(PTR_GC) const RootObject *thisPtr;

		// Thread to call on.
		Thread *thread;

		// Storage for target. Two words are enough for now.
		enum { targetSize = 2 };
		size_t target0;
		size_t target1;

		// Get target.
		inline FnTarget *target() const { return (FnTarget *)&target0; }

		// Compute which thread we want to run on.
		Thread *runOn(const TObject *first) const;
	};


	// Declare the template to Storm.
	STORM_TEMPLATE(Fn, createFn);


	/**
	 * Get 'x' as a TObject.
	 * Note: regular overloads have higher priority than templates as long as no implicit conversion have to be made.
	 */
	template <class T>
	inline const TObject *asTObject(const T &) { return null; }
	inline const TObject *asTObject(const TObject &t) { return &t; }
	inline const TObject *asTObject(const TObject *t) { return t; }

	// Helper macro to add a cloned object to a os::FnParams object. We need to keep the cloned
	// value alive until the function is actually called, so we can not use a function.
#define FN_ADD_CLONE(T, to, obj, env)				\
	typename RemoveConst<T>::Type tmp_ ## obj = obj; \
	cloned(tmp_ ## obj, env);							\
	to.add(tmp_ ## obj);

	/**
	 * C++ implementation. Supports up to two parameters.
	 *
	 * Note: there is an additional restriction from C++. We can not call functions which takes references.
	 */
	template <class R, class P1 = void, class P2 = void>
	class Fn : public FnBase {
		STORM_SPECIAL;
	public:
		// Get the Storm type.
		static Type *stormType(Engine &e) {
			return runtime::cppTemplate(e, FnId, 3, StormInfo<R>::id(), StormInfo<P1>::id(), StormInfo<P2>::id());
		}

		// Create.
		Fn(R (CODECALL *ptr)(P1, P2), Thread *thread = null) : FnBase(address(ptr), null, false, thread) {}

		template <class Q>
		Fn(R (CODECALL Q::*ptr)(P1, P2), const Q *obj) : FnBase(address(ptr), obj, true, null) {}

		// Call the function.
		R call(P1 p1, P2 p2) const {
			const TObject *first = asTObject(p1);

			if (needsCopy(first)) {
				CloneEnv *env = new (this) CloneEnv();
				os::FnStackParams<3> params;
				FN_ADD_CLONE(P1, params, p1, env);
				FN_ADD_CLONE(P2, params, p2, env);
				return callRaw<R>(params, first, env);
			} else {
				os::FnStackParams<3> params;
				params.add(p1);
				params.add(p2);
				return callRaw<R>(params, first, null);
			}
		}
	};

	/**
	 * 1 parameter.
	 */
	template <class R, class P1>
	class Fn<R, P1, void> : public FnBase {
		STORM_SPECIAL;
	public:
		// Get the Storm type.
		static Type *stormType(Engine &e) {
			return runtime::cppTemplate(e, FnId, 2, StormInfo<R>::id(), StormInfo<P1>::id());
		}

		// Create.
		Fn(R (CODECALL *ptr)(P1), Thread *thread = null) : FnBase(address(ptr), null, false, thread) {}

		template <class Q>
		Fn(R (CODECALL Q::*ptr)(P1), const Q *obj) : FnBase(address(ptr), obj, true, null) {}

		// Call the function.
		R call(P1 p1) const {
			const TObject *first = asTObject(p1);

			if (needsCopy(first)) {
				CloneEnv *env = new (this) CloneEnv();
				os::FnStackParams<2> params;
				FN_ADD_CLONE(P1, params, p1, env);
				return callRaw<R>(params, first, env);
			} else {
				os::FnStackParams<2> params;
				params.add(p1);
				return callRaw<R>(params, first, null);
			}
		}
	};

	/**
	 * 0 parameters.
	 */
	template <class R>
	class Fn<R, void, void> : public FnBase {
		STORM_SPECIAL;
	public:
		// Get the Storm type.
		static Type *stormType(Engine &e) {
			return runtime::cppTemplate(e, FnId, 1, StormInfo<R>::id());
		}

		// Create.
		Fn(R (CODECALL *ptr)(), Thread *thread = null) : FnBase(address(ptr), null, false, thread) {}

		template <class Q>
		Fn(R (CODECALL Q::*ptr)(), const Q *obj) : FnBase(address(ptr), obj, true, null) {}

		// Call the function.
		R call() const {
			if (needsCopy(null)) {
				CloneEnv *env = new (this) CloneEnv();
				os::FnStackParams<1> params;
				return callRaw<R>(params, null, env);
			} else {
				os::FnStackParams<1> params;
				return callRaw<R>(params, null, null);
			}
		}
	};


	/**
	 * Create easily.
	 */

	template <class R>
	Fn<R> *fnPtr(Engine &e, R (*fn)(), Thread *t = null) {
		return new (e) Fn<R>(fn, t);
	}

	template <class R, class P1>
	Fn<R, P1> *fnPtr(Engine &e, R (*fn)(P1), Thread *t = null) {
		return new (e) Fn<R, P1>(fn, t);
	}

	template <class R, class P1, class P2>
	Fn<R, P1, P2> *fnPtr(Engine &e, R (*fn)(P1, P2), Thread *t = null) {
		return new (e) Fn<R, P1, P2>(fn, t);
	}


	template <class R, class Q>
	Fn<R> *fnPtr(Engine &e, R (CODECALL Q::*fn)(), const Q *obj) {
		return new (e) Fn<R>(fn, obj);
	}

	template <class R, class Q, class P1>
	Fn<R, P1> *fnPtr(Engine &e, R (CODECALL Q::*fn)(P1), const Q *obj) {
		return new (e) Fn<R>(fn, obj);
	}

	template <class R, class Q, class P1, class P2>
	Fn<R, P1, P2> *fnPtr(Engine &e, R (CODECALL Q::*fn)(P1, P2), const Q *obj) {
		return new (e) Fn<R>(fn, obj);
	}

}
