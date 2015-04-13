#pragma once
#include "Object.h"
#include "Thread.h"
#include "Code/Reference.h"
#include "Code/FnParams.h"

namespace storm {

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

	/**
	 * Base class for a function.
	 */
	class FnPtrBase : public Object {
		STORM_CLASS;
	public:
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
			callRaw(output, typeInfo<R>(), params);
			R *result = (R *)d;
			R copy = *result;
			result->~R();
			return copy;
		}

		// Call function with a pointer to the return value.
		void callRaw(void *output, BasicTypeInfo type, const code::FnParams &params) const;

	protected:
		// Function to call.
		code::Ref fnRef;

		// Thread to execute on.
		Auto<Thread> thread;

		// This ptr (if needed).
		Object *thisPtr;

		// Strong this pointer?
		bool strongThisPtr;

		// Do we need to copy the parameters?
		inline bool needsCopy() {
			if (thread)
				return true;
			else
				return false;
		}

	};

	/**
	 * Compatible C++ class. Up to 3 parameters currently.
	 * In future releases, this may be implemented using variadic templates.
	 */
	template <class R, class P1 = void>
	class FnPtr : public FnPtrBase {
		// TYPE_EXTRA_CODE;
	public:
		// static Type *stormType(Engine &e) { return; }
		// static Type *stormType(const Object *o) { return; }

		R call(P1 p1) {
			if (needsCopy()) {
				typename AsAuto<P1>::v a = p1;
				clone(v);

				code::FnParams params;
				params.add(borrow(a));
				return callRaw<R>(params);
			} else {
				code::FnParams params;
				params.add(p1);
				return callRaw<R>(params);
			}
		}
	};

	template <class R>
	class FnPtr<R, void> {
	public:
		R call() {
			return callRaw<R>(code::FnParams());
		}
	};

}
