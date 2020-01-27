#pragma once

namespace storm {

	/**
	 * Low-level exception handling.
	 *
	 * Hooks into the exception handling system to make sure that exceptions thrown "by pointer" are
	 * properly scanned by the GC.
	 *
	 * The structure is global, and assumes that there are fairly few exceptions in flight at any
	 * given time. It will, however, work for any number of exceptions (since they may be kept alive
	 * eg. by exception_ptr), but performance may not be optimal since this is not a very common
	 * scenario.
	 *
	 * Due to the nature of garbage collectors, this structure needs to be updated in such a way
	 * that it is always possible to read a consistent view of the representation at all times. It
	 * is, however, fine to require locks when the structure is being modified (which we do).
	 */

	// Track an exception.
	void trackException(void *ptr, size_t size);

	// Stop tracking an exception.
	void untrackException(void *ptr);

	/**
	 * The following definitions are not indended to be used from outside this file, but are
	 * required to be public due to the templated scanning function below.
	 */

	/**
	 * A single exception.
	 */
	struct HeapException {
		// Location in memory.
		void *ptr;

		// Size of the allocation (in bytes).
		size_t size;
	};

	// Default size for the statically allocated part.
	enum { heapExStaticCount = 40 };


	/**
	 * A list of exceptions.
	 *
	 * There is one global list of exceptions that is sized to work for most use-cases. This may
	 * also be dynamically extended to include more memory if necessary.
	 */
	struct HeapExceptions {
		// Number of elements.
		size_t count;

		// Number of slots filled.
		size_t filled;

		// Last slot filled. We use this as a hint as to where empty locations might be found.
		size_t hint;

		// Data (array is filled to 'count' elements in the dynamically allocated version).
		HeapException data[heapExStaticCount];
	};


	// The static list.
	extern HeapExceptions staticExceptions;

	// The dynamic list. Might be null.
	extern HeapExceptions *dynamicExceptions;

	// Scan all pointers in a particular list of exceptions. Use the one without additional parameters instead.
	template <class Scanner>
	typename Scanner::Result scanExceptions(typename Scanner::Source &source, HeapExceptions *list) {
		Scanner s(source);
		typename Scanner::Result r = typename Scanner::Result();

		// Skip scanning alltogether if it is empty.
		if (!list || list->filled == 0)
			return r;

		for (size_t i = 0; i < list->count; i++) {
			HeapException &ex = list->data[i];
			if (!ex.ptr)
				continue;

			// Scan it, word by word.
			for (size_t offset = 0; offset < ex.size; offset += sizeof(void *)) {
				void *ptrLoc = (byte *)ex.ptr + offset;
				void **ptr = (void **)ptrLoc;
				if (s.fix1(*ptr)) {
					r = s.fix2(ptr);
					if (r != typename Scanner::Result())
						return r;
				}
			}
		}

		return r;
	}

	// Scan all pointers in the exception lists. Expected to be called by a garbage collector during
	// scanning. As such, this function does not take any locks.
	template <class Scanner>
	typename Scanner::Result scanExceptions(typename Scanner::Source &source) {
		typename Scanner::Result r = scanExceptions<Scanner>(source, &staticExceptions);
		if (r != typename Scanner::Result())
			return r;
		return scanExceptions<Scanner>(source, atomicRead(dynamicExceptions));
	}

}
