#include "stdafx.h"
#include "VM.h"

#if STORM_GC == STORM_GC_SMM

namespace storm {
	namespace smm {

#if defined(WINDOWS)

		// Flags that applies to all allocations.
		static const DWORD allocFlags = 0;
		static const DWORD allocProt = PAGE_EXECUTE_READWRITE;

		size_t vmPageSize() {
			SYSTEM_INFO info;
			GetSystemInfo(&info);
			return info.dwPageSize;
		}

		size_t vmAllocGranularity() {
			SYSTEM_INFO info;
			GetSystemInfo(&info);
			return info.dwAllocationGranularity;
		}

		void *vmAlloc(size_t size) {
			return VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE | allocFlags, allocProt);
		}

		void vmFree(void *mem, size_t size) {
			(void)size;
			VirtualFree(mem, 0, MEM_RELEASE);
		}

#elif defined (POSIX)

#error "Please implement the virtual memory functions for Posix!"

#else
#error "Please implement the virtual memory functions for your platform!"
#endif

	}
}

#endif
