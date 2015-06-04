#pragma once
#include "Object.h"
#include "TObject.h"
#include "Shared/CloneEnv.h"
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
	 *
	 * Function pointers have a slightly different call semantics from regular function calls
	 * in Storm. This is because we do not have the same amount of information when dealing
	 * with function pointers. If parameters are copied or not depends on wether the thread
	 * is the same as the currently running thread or not, this is decided runtime.
	 *
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
		// Create. 'thisPtr' may be null.
		FnPtrBase(const code::Ref &ref, Par<Thread> thread, bool member, Object *thisPtr = null, bool strong = false);

		// Create with C++ fn. If 'thisPtr' is set, 'fn' is assumed to be a member function. This is a restriction
		// in the C++ api, since there is no other way of knowing.
		FnPtrBase(const void *fn, Object *thisPtr = null, bool strongThis = false);

		// Copy.
		STORM_CTOR FnPtrBase(Par<FnPtrBase> o);

		// Destroy.
		~FnPtrBase();

		// Helper to create FnPtrs.
		static FnPtrBase *CODECALL createRaw(Type *type, void *refData,
											Thread *t, Object *thisPtr,
											bool strong, bool member);

		// Deep copy.
		virtual void STORM_FN deepCopy(Par<CloneEnv> env);

		// Call our function (low-level, not type safe).
		// Takes care of delegating to another thread if needed, and adds a this pointer if needed.
		// The 'first' parameter is the first parameter as a TObject (if the first parameter is a TObject)
		// and is used to decide if a thread call is to be made or not.
		// Note: Does _not_ copy parameters if needed, but handles the return value.
		template <class R>
		R callRaw(const code::FnParams &params, TObject *first) const {
			byte d[sizeof(R)];
			callRaw(d, typeInfo<R>(), params, first);
			R *result = (R *)d;
			R copy = *result;
			result->~R();

			if (needsCopy(first)) {
				Auto<CloneEnv> env = CREATE(CloneEnv, this);
				clone(copy, env);
			}
			return copy;
		}

		// Call function with a pointer to the return value.
		void callRaw(void *output, const BasicTypeInfo &type, const code::FnParams &params, TObject *first) const;

		// Do we need to copy the parameters? 'first' is the first parameter to the call, as a TObject.
		bool needsCopy(TObject *first) const;

	protected:
		// Function to call.
		code::Ref fnRef;

		// Raw function pointer (if fnRef points to nothing).
		const void *rawFn;

		// The thread the function should be executed on. If 'thisPtr' is null, there may be
		// a thread anyway, but that will not be known until we see the first parameter.
		Auto<Thread> thread;

		// This ptr (if needed).
		Object *thisPtr;

		// Strong this pointer?
		bool strongThisPtr;

		// Is this function a member function?
		bool isMember;

		// Init.
		void init();

		// Which thread shall we run on?
		Thread *runOn(TObject *first) const;

		// Output.
		virtual void output(wostream &to) const;
	};

	// Get a parameter as a TObject if applicable.
	template <class T>
	inline TObject *asTObject(const T &) { return null; }

	// Note: regular overloads have higher priority over templates as long as no implicit conversion
	// has to be made.
	inline TObject *asTObject(TObject *o) { return o; }
	inline TObject *asTObject(Par<TObject> o) { return o.borrow(); }
	inline TObject *asTObject(Auto<TObject> o) { return o.borrow(); }

	// Helper macro for adding a copied parameter
#define ADD_COPY(to, type, param, env)				\
	typename AsAuto<type>::v t ## param = param;	\
	clone(t ## param, env);							\
	to.add(borrow(t ## param));

	/**
	 * Compatible C++ class. Up to 2 parameters currently.
	 * In future releases, this may be implemented using variadic templates.
	 */
	template <class R, class P1 = void, class P2 = void>
	class FnPtr : public FnPtrBase {
		TYPE_EXTRA_CODE;
	public:
		static Type *stormType(Engine &e) { return fnPtrType(e, valList(3, value<R>(e), value<P1>(e), value<P2>(e))); }

		// Create.
		FnPtr(const void *r, Object *thisPtr, bool strongThis) : FnPtrBase(r, thisPtr, strongThis) {}

		R call(P1 p1, P2 p2) {
			TObject *first = asTObject(p1);

			if (needsCopy(first)) {
				Auto<CloneEnv> env = CREATE(CloneEnv, engine());
				code::FnParams params;

				ADD_COPY(params, P1, p1, env);
				ADD_COPY(params, P2, p2, env);

				return callRaw<R>(params, first);
			} else {
				code::FnParams params;
				params.add(p1);
				params.add(p2);
				return callRaw<R>(params, first);
			}
		}
	};

	template <class R, class P1>
	class FnPtr<R, P1, void> : public FnPtrBase {
		TYPE_EXTRA_CODE;
	public:
		static Type *stormType(Engine &e) { return fnPtrType(e, valList(2, value<R>(e), value<P1>(e))); }

		// Create.
		FnPtr(const void *r, Object *thisPtr, bool strongThis) : FnPtrBase(r, thisPtr, strongThis) {}

		R call(P1 p1) {
			TObject *first = asTObject(p1);

			if (needsCopy(first)) {
				Auto<CloneEnv> env = CREATE(CloneEnv, engine());
				code::FnParams params;

				ADD_COPY(params, P1, p1, env);

				return callRaw<R>(params, first);
			} else {
				code::FnParams params;
				params.add(p1);
				return callRaw<R>(params, first);
			}
		}
	};

	template <class R>
	class FnPtr<R, void, void> : public FnPtrBase {
		TYPE_EXTRA_CODE;
	public:
		static Type *stormType(Engine &e) { return fnPtrType(e, valList(1, value<R>(e))); }

		FnPtr(const void *r, Object *thisPtr, bool strongThis) : FnPtrBase(r, thisPtr, strongThis) {}

		R call() {
			return callRaw<R>(code::FnParams(), null);
		}
	};


	// Helper macro to not get the preprocessor to choke on things like CREATE(FnPtr<A, B>);
#define FN_PTR(...) FnPtr<__VA_ARGS__>

	// Create functions. The CREATE macro does not play well with templates where there is more
	// than one parameter (problem with the pre-processor).

	// Note that all these fnPtr does not respect any threading mode on static functions. Therefore:
	// be careful when creating pointers to non-member functions with a threading directive!
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
	FnPtr<R, P1> *memberWeakPtr(Engine &e, C *thisPtr, R (CODECALL C::*fn)(P1)) {
		return CREATE(FN_PTR(R, P1), e, address(fn), thisPtr, false);
	}

	template <class R, class C>
	FnPtr<R> *memberWeakPtr(Engine &e, C *thisPtr, R (CODECALL C::*fn)()) {
		return CREATE(FN_PTR(R), e, address(fn), thisPtr, false);
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
