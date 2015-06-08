#pragma once
#include "Object.h"
#include "Handle.h"
#include "Storm/Value.h"
#include "OS/Future.h"
#include "OS/Sync.h"

namespace storm {
	STORM_PKG(core);

	// Implemented in FutureTemplate.cpp, looks up a specific specialization.
	Type *futureType(Engine &e, const Value &type);

	/**
	 * Future-object exposed to the runtime and built into function calls.
	 * By calling 'asyncThreadCall' on a function, you will get one of these
	 * objects. It also simplifies the interface of the Future in code since
	 * it supports copying of the future (references the same result) and also
	 * correctly handles cases where the result is not waited for.
	 */
	class FutureBase : public Object {
		STORM_CLASS;
	public:
		// Create.
		FutureBase(const Handle &type);

		// Copy ctor.
		FutureBase(const FutureBase *o);

		// Destroy.
		~FutureBase();

		// Deep copy.
		virtual void STORM_FN deepCopy(Par<CloneEnv> env);

		// Post a result.
		void CODECALL postRaw(const void *value);

		// Post an error (assuming we're inside a catch-block).
		void error();

		// Wait for the result. 'to' is empty memory where the value will be copied into.
		void CODECALL resultRaw(void *to);

		// Get the underlying future object. Note: when you call this function,
		// you are required to call either postRaw or error on the future object, otherwise
		// we will leak resources!
		os::FutureBase *rawFuture();

	private:
		// Custom extension of the 'FutureSema' object, so that we may get a notification when a
		// result has been posted.
		class FutureSema : public os::FutureSema<Sema> {
		public:
			// Ctor.
			FutureSema(void *data);

		protected:
			// Custom notify so that we can keep the Data struct alive long enough.
			virtual void notify();
		};

		// Data shared between futures. Since we allow copies, we need to share one
		// FutureBase object.
		struct Data {
			// Number of references to this object.
			nat refs;

			// What type are we playing with?
			const Handle *handle;

			// The Future object we are playing with.
			FutureSema future;

			// Do a release whenever the result has been posted?
			nat releaseOnResult;

			// Memory for the returned value. This one is not initialized until
			// the future has got a result at least once. Note that it is allocated
			// inline with the struct, and therefore it needs to be the last element.
			byte data[1];

			// Allocate.
			static Data *alloc(const Handle &type);

			// Deallocate.
			void free();

			// Add/release ref.
			void addRef();
			void release();

			// Called when a result has been posted.
			static void resultPosted(FutureSema *from);
		};

		// Our data.
		Data *data;
	};

	/**
	 * Specialized class, compatible with the template instantiations generated
	 * by the Storm runtime.
	 */
	template <class T>
	class Future : public FutureBase {
		TYPE_EXTRA_CODE;
	public:
		// Type lookup.
		static Type *stormType(Engine &e) { return futureType(e, value<T>(e)); }

		// Create.
		Future() : FutureBase(storm::handle<T>()) {}

		// Copy.
		Future(const Future<T> *o) : FutureBase(o) {}

		// Post result.
		void post(const T &t) {
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
		TYPE_EXTRA_CODE;
	public:
		// Type lookup.
		static Type *stormType(Engine &e) { return futureType(e, Value()); }

		// Create.
		Future() : FutureBase(storm::handle<void>()) {}

		// Copy.
		Future(const Future<void> *o) : FutureBase(o) {}

		// Post.
		void post() {
			postRaw(null);
		}

		// Get result.
		void result() {
			resultRaw(null);
		}
	};


	/**
	 * Use a future of pointer, equal to Future<Auto<T>>
	 */
	template <class T>
	class FutureP : public Future<Auto<T> > {
	public:
		// Create.
		FutureP() {}

		// Copy.
		FutureP(const FutureP<T> *o) : Future(o) {}
	};

}
