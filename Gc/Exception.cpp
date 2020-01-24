#include "stdafx.h"
#include "Exception.h"
#include "Gc.h"
#include "Utils/Lock.h"

namespace storm {

	// Static exceptions (zero initialized by the compiler).
	HeapExceptions staticExceptions = { heapExStaticCount, 0, 0 };

	// Dynamically managed exceptions.
	HeapExceptions *dynamicExceptions = null;

	// Lock when they are modified.
	static util::Lock exLock;

	// Try to add an exception to a HeapExceptions structure. Will not attempt to resize.
	static bool add(HeapExceptions *to, void *ptr, size_t size) {
		// Full?
		if (!to || to->filled == to->count)
			return false;

		size_t hint = to->hint;
		while (to->data[hint].ptr) {
			if (++hint > to->count)
				hint = 0;
		}

		// We found an empty location!
		to->hint = hint;

		// We need to make sure to write the size first. Otherwise, a scanner might use the wrong
		// size when scanning.
		atomicWrite(to->data[hint].size, size);
		atomicWrite(to->data[hint].ptr, ptr);

		// Update 'filled'. Make sure we read reasonable values from a scanner.
		atomicIncrement(to->filled);

		return true;
	}

	// Grow the dynamic list of exceptions. Calls 'terminate' on failure.
	static void growDynamic() {
		size_t newCount = heapExStaticCount;
		HeapExceptions *old = dynamicExceptions;
		if (old) {
			// Grow linearly. We don't expect a massive increase in size.
			newCount = old->count + heapExStaticCount;
		}

		size_t newSize = sizeof(HeapExceptions) + sizeof(HeapExceptions)*(newCount - heapExStaticCount);
		HeapExceptions *alloc = (HeapExceptions *)malloc(newSize);
		memset(alloc, 0, newSize);
		alloc->count = newCount;

		// Copy the old content, if applicable. Note: We don't need to be careful about the contents
		// of the new one as it is not made public yet.
		if (old) {
			for (size_t i = 0; i < old->count; i++) {
				if (old->data[i].ptr)
					alloc->data[alloc->hint++] = old->data[i];
			}
		}

		// Publish the new one.
		atomicWrite(dynamicExceptions, alloc);

		// Now, we can kill the old one. We know that the GC is not scanning the old one at this
		// point since the GC will pause all threads during scanning. Otherwise, we would need to
		// have a more complicated protocol for hand-offs.
		free(old);
	}

	// Remove an element.
	static bool remove(HeapExceptions *from, void *ptr) {
		if (!from || from->filled == 0)
			return false;

		for (size_t i = 0; i < from->count; i++) {
			if (from->data[i].ptr == ptr) {
				// Be careful with clearing the pointer.
				atomicWrite(from->data[i].ptr, (void *)null);
				atomicWrite(from->data[i].size, (size_t)0);
				return true;
			}
		}

		return false;
	}

	void trackException(void *ptr, size_t size) {
		util::Lock::L z(exLock);

		if (add(&staticExceptions, ptr, size))
			return;

		if (add(dynamicExceptions, ptr, size))
			return;

		// We need to re-size the dynamic part...
		growDynamic();

		// This *will* succeed.
		add(dynamicExceptions, ptr, size);
	}

	void untrackException(void *ptr) {
		util::Lock::L z(exLock);

		if (remove(&staticExceptions, ptr))
			return;

		if (remove(dynamicExceptions, ptr)) {
			// If the dynamic part is empty now, remove it!
			HeapExceptions *dyn = dynamicExceptions;
			if (dyn->filled == 0) {
				atomicWrite(dynamicExceptions, (HeapExceptions *)null);
				free(dyn);
			}
		}
	}


#if defined(WINDOWS)

	// Not needed on Windows, as long as we don't use std::exception_ptr to store GC allocated
	// exceptions (which we don't want to do on POSIX either). Thus, we only need to stub out the
	// API here.

#elif defined(POSIX)

	/**
	 * Hooks into the ABI.
	 */

	typedef void *(*AbiAllocEx)(size_t);
	typedef void (*AbiFreeEx)(void *);

	AbiAllocEx allocEx = null;
	AbiFreeEx freeEx = null;

	extern "C" SHARED_EXPORT void *__cxa_allocate_exception(size_t thrown_size) {
		if (!allocEx)
			allocEx = (AbiAllocEx)dlsym(RTLD_NEXT, "__cxa_allocate_exception");

		void *result = (*allocEx)(thrown_size);
		trackException(result, thrown_size);
		return result;
	}

	extern "C" SHARED_EXPORT void __cxa_free_exception(void *thrown_object) {
		if (!freeEx)
			freeEx = (AbiFreeEx)dlsym(RTLD_NEXT, "__cxa_free_exception");

		untrackException(thrown_object);
		return (*freeEx)(thrown_object);
	}

#endif

}
