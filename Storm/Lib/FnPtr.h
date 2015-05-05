#pragma once
#include "Object.h"
#include "CloneEnv.h"
#include "Storm/Value.h"
#include "Storm/Thread.h"
#include "Code/Reference.h"
#include "Code/FnParams.h"

namespace storm {
	STORM_PKG(core);

	/**
	 * Function pointers for Storm and C++ code. Function pointers to member
	 * functions contains a this pointer as well. This function pointer is
	 * currently weak by default, but may be extended to contain strong references
	 * in the future.
	 * Function pointers in Storm also contain information about which thread should
	 * be used for the function. This information is fetched at the time the
	 * function pointer is created, since the thread associated with objects are
	 * constants over the object's lifetime. We are also locked to a specific
	 * this-pointer, which makes this approach safe.
	 * Function pointers to values are not yet supported.
	 * TODO: The weak references should be checked!
	 */

	// Find the function pointer type. Implemented in FnPtrTemplate.cpp
	Type *fnPtrType(Engine &e, const vector<Value> &params);

	/**
	 * Base class for a function.
	 */
	class FnPtrBase : public Object {
		STORM_CLASS;
	public:
		// Create.
		FnPtrBase(const code::Ref &ref, Object *thisPtr = null, bool strongThis = false);

		// Create with C++ fn.
		FnPtrBase(const void *fn, Object *thisPtr = null, bool strongThis = false);

		// Copy.
		STORM_CTOR FnPtrBase(Par<FnPtrBase> o);

		// Destroy.
		~FnPtrBase();

		// Deep copy.
		virtual void STORM_FN deepCopy(Par<CloneEnv> env);

		// Call our function (low-level, not type safe).
		// Takes care of delegating to another thread if needed, and
		// adds a this pointer if needed.
		// Note: Does _not_ copy parameters if needed!
		template <class R>
		R callRaw(const code::FnParams &params) const {
			byte d[sizeof(R)];
			callRaw(d, typeInfo<R>(), params);
			R *result = (R *)d;
			R copy = *result;
			result->~R();

			Auto<CloneEnv> env = CREATE(CloneEnv, engine());
			clone(copy, env);
			return copy;
		}

		// Call function with a pointer to the return value.
		void callRaw(void *output, const BasicTypeInfo &type, const code::FnParams &params) const;

		// Do we need to copy the parameters?
		inline bool needsCopy() {
			if (thread)
				return true;
			else
				return false;
		}

	protected:
		// Function to call.
		code::Ref fnRef;

		// Raw function pointer (if fnRef points to nothing).
		const void *rawFn;

		// Thread to execute on.
		Auto<Thread> thread;

		// This ptr (if needed).
		Object *thisPtr;

		// Strong this pointer?
		bool strongThisPtr;

		// Init.
		void init();

	};

	// Helper macro for adding a copied parameter
#define ADD_COPY(to, type, param, env)				\
	typename AsAuto<type>::v t ## param = param;	\
	clone(t ## param, env);							\
	to.add(borrow(t ## param));

	/**
	 * Compatible C++ class. Up to 3 parameters currently.
	 * In future releases, this may be implemented using variadic templates.
	 */
	template <class R, class P1 = void>
	class FnPtr : public FnPtrBase {
		TYPE_EXTRA_CODE;
	public:
		static Type *stormType(Engine &e) { return fnPtrType(e, valList(2, value<R>(e), value<P1>(e))); }

		// Create.
		FnPtr(const code::Ref &r, Object *thisPtr, bool strongThis) : FnPtrBase(r, thisPtr, strongThis) {}
		FnPtr(const void *r, Object *thisPtr, bool strongThis) : FnPtrBase(r, thisPtr, strongThis) {}

		R call(P1 p1) {
			if (needsCopy()) {
				Auto<CloneEnv> env = CREATE(CloneEnv, engine());
				code::FnParams params;

				ADD_COPY(params, P1, p1, env);

				return callRaw<R>(params);
			} else {
				code::FnParams params;
				params.add(p1);
				return callRaw<R>(params);
			}
		}
	};

	template <class R>
	class FnPtr<R, void> : public FnPtrBase {
		TYPE_EXTRA_CODE;
	public:
		static Type *stormType(Engine &e) { return fnPtrType(e, valList(1, value<R>(e))); }

		FnPtr(const code::Ref &r, Object *thisPtr, bool strongThis) : FnPtrBase(r, thisPtr, strongThis) {}
		FnPtr(const void *r, Object *thisPtr, bool strongThis) : FnPtrBase(r, thisPtr, strongThis) {}

		R call() {
			return callRaw<R>(code::FnParams());
		}
	};


	// Helper macro to not get the preprocessor to choke on things like CREATE(FnPtr<A, B>);
#define FN_PTR(...) FnPtr<__VA_ARGS__>

	// Create functions. The CREATE macro does not play well with templates where there is more
	// than one parameter (problem with the pre-processor).
	template <class R, class P1>
	FnPtr<R, P1> *fnPtr(Engine &e, R (*fn)(P1)) {
		return CREATE(FN_PTR(R, P1), e, fn, null, false);
	}

	template <class R>
	FnPtr<R> *fnPtr(Engine &e, R (*fn)()) {
		return CREATE(FN_PTR(R), e, fn, null, false);
	}

	template <class R, class P1, class C>
	FnPtr<R, P1> *memberWeakPtr(Engine &e, Par<C> thisPtr, R (CODECALL C::*fn)(P1)) {
		return CREATE(FN_PTR(R, P1), e, address(fn), thisPtr.borrow(), false);
	}

	template <class R, class C>
	FnPtr<R> *memberWeakPtr(Engine &e, Par<C> thisPtr, R (CODECALL C::*fn)()) {
		return CREATE(FN_PTR(R), e, address(fn), thisPtr.borrow(), false);
	}

	template <class R, class P1, class C>
	FnPtr<R, P1> *memberWeakPtr(Engine &e, Auto<C> thisPtr, R (CODECALL C::*fn)(P1)) {
		return CREATE(FN_PTR(R, P1), e, address(fn), thisPtr.borrow(), false);
	}

	template <class R, class C>
	FnPtr<R> *memberWeakPtr(Engine &e, Auto<C> thisPtr, R (CODECALL C::*fn)()) {
		return CREATE(FN_PTR(R), e, address(fn), thisPtr.borrow(), false);
	}


#undef FN_PTR

}
