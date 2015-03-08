#pragma once
#include "Utils/Semaphore.h"

// In Visual Studio 2008, we do not have exception_ptr, so we implement our own
// solution in that case!
#if VS <= 2008
#define CUSTOM_EXCEPTION_PTR
#else
#include <stdexcept>
#endif

namespace code {
#ifdef CUSTOM_EXCEPTION_PTR
	struct CppExceptionType;
#endif

	/**
	 * Future-like class that delivers either the result or an
	 * exception between two threads.
	 */
	class Future : NoCopy {
	public:
		// Create the future, indicating a location for the result.
		Future(void *target, nat size);

		// Wait for the result. This function will either return
		// normally, indicating the result is written to 'target',
		// or throw an exception posted.
		void wait();

		// Get the location for the target value.
		inline void *target() { return targetPtr; }

		// Tell the waiting thread we have posted a result.
		void result();

		// Post an error. This function must be called from within a
		// throw-catch block. Otherwise the runtime crashes.
		void error();

	private:
		// Target value.
		void *targetPtr;

		// Target size.
		nat size;

		// Semaphores for waiting.
		Semaphore resultSema;

		// Any error?
		bool hasError;

		// Throw the error captured earlier.
		void throwError();

#ifdef CUSTOM_EXCEPTION_PTR
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
#endif
	};

}
