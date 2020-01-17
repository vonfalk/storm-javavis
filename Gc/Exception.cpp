#include "stdafx.h"
#include "Exception.h"
#include "Gc.h"

namespace storm {

#if defined(WINDOWS)

	// Not needed on Windows, as long as we don't use std::exception_ptr to store GC allocated
	// exceptions (which we don't want to do on POSIX either). Thus, we only need to stub out the
	// API here.

#elif defined(POSIX)

	typedef void *(*AbiAllocEx)(size_t);
	typedef void (*AbiFreeEx)(void *);

	AbiAllocEx allocEx = null;
	AbiFreeEx freeEx = null;

	extern "C" SHARED_EXPORT void *__cxa_allocate_exception(size_t thrown_size) {
		if (!allocEx)
			allocEx = (AbiAllocEx)dlsym(RTLD_NEXT, "__cxa_allocate_exception");

		void *result = (*allocEx)(thrown_size);
		PLN(L"Allocated exception " << result << L", " << thrown_size << L" bytes.");
		return result;
	}

	extern "C" SHARED_EXPORT void __cxa_free_exception(void *thrown_object) {
		if (!freeEx)
			freeEx = (AbiFreeEx)dlsym(RTLD_NEXT, "__cxa_free_exception");

		PLN(L"Freeing exception " << thrown_object << L"!");
		return (*freeEx)(thrown_object);
	}

#endif

}
