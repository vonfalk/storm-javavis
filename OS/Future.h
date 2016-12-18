#pragma once
#include "Utils/TypeInfo.h"
#include "Utils/Semaphore.h"

// In Visual Studio 2008, we do not have exception_ptr, so we implement our own
// solution in that case!
#if VISUAL_STUDIO <= 2008
#define CUSTOM_EXCEPTION_PTR
#else
#include <stdexcept>
#endif

namespace os {
#ifdef CUSTOM_EXCEPTION_PTR
	struct CppExceptionType;
#endif

	/**
	 * Future that stores the result in some unrelated memory. This class is designed to be possible
	 * to use with minimal type information and to be easy to use from machine code or general
	 * non-templated C++ code. Therefore it is not very type-safe at all. For a more type-safe and
	 * user-friendly version, see the Future<> class below. The default implementation of FutureBase
	 * is this one (specialized with <void>). This is abstract, as it requires the use of a specific
	 * semaphore type.
	 */
	class FutureBase : NoCopy {
	public:
		// Create the future, indicating a location for the result.
		FutureBase();

		// Detect unhandled exceptions and print them.
		~FutureBase();

		// Wait for the result. This function will either return
		// normally, indicating the result is written to 'target',
		// or throw an exception posted.
		void result();

		// Tell the waiting thread we have posted a result.
		void posted();

		// Post an error. This function must be called from within a
		// throw-catch block. Otherwise the runtime crashes.
		void error();

		// Has any result been posted?
		bool dataPosted();

		// Any result (either data or error) posted?
		bool anyPosted();

	protected:
		// Manage the notification between threads (whichever implementation is used).
		virtual void notify() = 0;
		virtual void wait() = 0;

	private:

		// Throw the error captured earlier.
		void throwError();

		// Save the current exception.
		void saveError();

#ifdef CUSTOM_EXCEPTION_PTR
		// Any error?
		inline bool hasError() const {
			return errorData.data != null;
		}

		class Cloned : NoCopy {
		public:
			Cloned();
			~Cloned();

			void *data;
			const CppExceptionType *type;

			// Rethrow this exception.
			void rethrow();

			// Clear the data here.
			void clear();

			// Copy from an exception.
			void set(void *from, const CppExceptionType *type);
		};

		// The exception that should be passed to the other thread.
		Cloned errorData;

		// Filter expression function.
		int filter(EXCEPTION_POINTERS *ptrs);

#else
		// Exception.
		std::exception_ptr errorData;

		// Any error?
		inline bool hasError() const {
			return errorData;
		}
#endif

		// Anything posted? 0 or 1
		nat resultPosted;

		// Anything read?
		nat resultRead;
	};

	/**
	 * Specify which semaphore to use. T is either Semaphore or Sema, or something compatible.
	 */
	template <class T>
	class FutureSema : public FutureBase {
	public:
		// Create, just like above.
		FutureSema() : FutureBase(), sema(0) {}

	protected:
		// The semaphore to use.
		T sema;

		virtual void wait() {
			sema.down();
			sema.up(); // allows us to wait more than once!
		}

		virtual void notify() {
			sema.up();
		}
	};

	/**
	 * A real C++ friendly implementation of the future. Defaults to use the
	 * semaphore with support for background threads. This class does not
	 * inherit from the FutureBase<> class to provide a cleaner interface
	 * to the user. The underlying FutureBase<> object may still be queried
	 * if neccessary.
	 * This class assumes that you will call 'result' at least once.
	 */
	template <class T, class Sema = os::Sema>
	class Future : NoCopy {
	public:
		// Create.
		Future() {}

		// Destroy.
		~Future() {
			if (future.dataPosted()) {
				((T *)value)->~T();
			}
		}

		// Get the result or throw the error, when the thread is ready.
		T result() {
			future.result();
			return *(T *)value;
		}

		// Post the result.
		void post(const T &result) {
			new (value) T(result);
			future.posted();
		}

		// Post an error. Only call from a catch-block.
		void error() {
			future.error();
		}

		// Get the underlying Future object.
		FutureBase &impl() {
			return future;
		}

		// Get our data.
		void *data() {
			return value;
		}

	private:
		// Underlying object.
		FutureSema<Sema> future;

		// Result storage. Note that we do not want to initialize it before
		// 'future' tries to return something into this.
		byte value[sizeof(T)];
	};

	/**
	 * Special case for void values (exceptions and syncronization may still be interesting!).
	 */
	template <class Sema>
	class Future<void, Sema> : NoCopy {
	public:
		// Create.
		Future() {}

		// Destroy.
		~Future() {}

		// Get the result or throw the error, when the thread is ready.
		void result() {
			future.result();
		}

		// Post the result.
		void post() {
			future.posted();
		}

		// Post an error. Only call from a catch-block.
		void error() {
			future.error();
		}

		// Get the underlying Future object.
		FutureBase &impl() {
			return future;
		}

		// Get our data.
		void *data() {
			return null;
		}

	private:
		// Underlying object.
		FutureSema<Sema> future;
	};

}
