#include "stdafx.h"
#include "Exception.h"
#include "Gc.h"

namespace storm {

#if defined(WINDOWS)

#error "Not implemented yet!"

#elif defined(POSIX)

	typedef void *(*AbiAllocEx)(size_t);
	typedef void (*AbiFreeEx)(void *);

	AbiAllocEx allocEx = null;
	AbiFreeEx freeEx = null;

	extern "C" SHARED_EXPORT void *__cxa_allocate_exception(size_t thrown_size) {
		if (!allocEx)
			allocEx = (AbiAllocEx)dlsym(RTLD_NEXT, "__cxa_allocate_exception");

		void *result = (*allocEx)(thrown_size);
		printf("Allocated exception %p, %d bytes\n", result, (int)thrown_size);
		return result;
	}

	extern "C" SHARED_EXPORT void __cxa_free_exception(void *thrown_object) {
		if (!freeEx)
			freeEx = (AbiFreeEx)dlsym(RTLD_NEXT, "__cxa_free_exception");

		printf("Freeing exception! %p\n", thrown_object);
		return (*freeEx)(thrown_object);
	}

#endif

}
