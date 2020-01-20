#pragma once
#include "Object.h"
#include "Handle.h"
#include "OS/Future.h"
#include "OS/Sync.h"

namespace storm {
	STORM_PKG(core);

	/**
	 * Future-object exposed to the runtime and built into function calls.  By calling
	 * 'asyncThreadCall' on a function, you will get one of these objects. It also simplifies the
	 * interface of the Future in code since it supports copying of the future (references the same
	 * result) and also correctly handles cases where the result is not waited for.
	 *
	 * The future takes care of any copying of the objects.
	 */
	class FutureBase : public Object {
		STORM_CLASS;
	public:
		// Create.
		FutureBase(const Handle &type);

		// Copy ctor.
		FutureBase(const FutureBase &o);

		// Destroy.
		~FutureBase();

		// Deep copy.
		virtual void STORM_FN deepCopy(CloneEnv *env);

		// Post a result.
		void CODECALL postRaw(const void *value);

		// Post an error (assuming we're inside a catch-block).
		void error();

		// Wait for the result. 'to' is empty memory where the value will be copied into.
		void CODECALL resultRaw(void *to);

		// Detach the future, ie. tell it we don't care about the result.
		void STORM_FN detach();

		// Get the underlying future object. Note: when you call this function,
		// you are required to call either postRaw or error on the future object, otherwise
		// we will leak resources!
		os::FutureBase *rawFuture();

		// Get the place where the result is to be stored.
		void *rawResult() { return result->v; }

	private:
		// Custom extension of the 'FutureSema' object, so that we may get a notification when a
		// result has been posted.
		class FutureSema : public os::FutureSema<os::Sema> {
		public:
			// Ctor.
			FutureSema();

		protected:
			// Custom notify so that we can keep the Data struct alive long enough.
			virtual void notify();
		};

		// Data shared between futures. Since we allow copies, we need to share one FutureBase
		// object. This struct is allocated on the non-moving GC heap since it contains potentially
		// sensitive constructs.
		struct Data {
			Data();
			~Data();

			// The Future object we are playing with.
			FutureSema future;

			// Number of references to this object.
			nat refs;

			// Do a release whenever the result has been posted?
			nat releaseOnResult;

			// Add/release ref. If 'release' returns true, then the last instance was freed.
			void addRef();
			bool release();

			// Called when a result has been posted.
			static void resultPosted(FutureSema *from);

			// GC description for this type.
			static const GcType gcType;
		};

		// Our data (non-gc).
		Data *data;

		// Handle.
		const Handle &handle;

		// Store the return value. We use 'filled' to see if something is stored here.
		GcArray<byte> *result;
	};

	// Declare the template.
	STORM_TEMPLATE(Future, createFuture);

	/**
	 * Specialized class, compatible with the template instantiations generated
	 * by the Storm runtime.
	 */
	template <class T>
	class Future : public FutureBase {
		STORM_SPECIAL;
	public:
		// Type lookup.
		static Type *stormType(Engine &e) {
			return runtime::cppTemplate(e, FutureId, 1, StormInfo<T>::id());
		}

		// Create.
		Future() : FutureBase(StormInfo<T>::handle(engine())) {
			runtime::setVTable(this);
		}

		// Copy.
		Future(const Future<T> *o) : FutureBase(o) {
			runtime::setVTable(this);
		}

		// Post result.
		void post(T t) {
			cloned(t, new (this) CloneEnv());
			postRaw(&t);
		}

		// Get result.
		T result() {
			byte data[sizeof(T)];
			resultRaw(data);
			T copy = *(T *)data;
			((T *)data)->~T();
			return copy;
		}
	};

	/**
	 * Special case for 'void'
	 */
	template <>
	class Future<void> : public FutureBase {
		STORM_SPECIAL;
	public:
		// Type lookup.
		static Type *stormType(Engine &e) {
			return runtime::cppTemplate(e, FutureId, 1, StormInfo<void>::id());
		}

		// Create.
		Future() : FutureBase(StormInfo<void>::handle(engine())) {
			runtime::setVTable(this);
		}

		// Copy.
		Future(const Future<void> &o) : FutureBase(o) {
			runtime::setVTable(this);
		}

		// Post.
		void post() {
			postRaw(null);
		}

		// Get result.
		void result() {
			resultRaw(null);
		}
	};

}
