#include "stdafx.h"
#include "VMWin.h"

#if STORM_GC == STORM_GC_SMM && defined(WINDOWS)

namespace storm {
	namespace smm {

		// Flags that applies to all allocations.
		static const DWORD allocFlags = 0;
		static const DWORD allocProt = PAGE_EXECUTE_READWRITE;

		VMWin *VMWin::create(size_t initialSize) {
			SYSTEM_INFO info;
			GetSystemInfo(&info);

			return new VMWin(info.dwPageSize, info.dwAllocationGranularity);
		}

		VMWin::VMWin(size_t pageSize, size_t granularity) : VM(pageSize, granularity) {}

		void *VMWin::alloc(size_t size) {
			return VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE | allocFlags, allocProt);
		}

		void VMWin::free(void *mem, size_t size) {
			VirtualFree(mem, 0, MEM_RELEASE);
		}

	}
}

#endif
