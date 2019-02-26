#include "stdafx.h"
#include "VMWin.h"

#if STORM_GC == STORM_GC_SMM && defined(WINDOWS)

namespace storm {
	namespace smm {

		// Flags that applies to all allocations.
		static const DWORD reserveFlags = MEM_WRITE_WATCH;
		static const DWORD commitFlags = 0;
		static const DWORD allocProt = PAGE_EXECUTE_READWRITE;

		VMWin *VMWin::create() {
			SYSTEM_INFO info;
			GetSystemInfo(&info);

			return new VMWin(info.dwPageSize, info.dwAllocationGranularity);
		}

		VMWin::VMWin(size_t pageSize, size_t granularity) : VM(pageSize, granularity) {}

		void *VMWin::reserve(void *at, size_t size) {
			return VirtualAlloc(at, size, MEM_RESERVE | reserveFlags, PAGE_NOACCESS);
		}

		void VMWin::commit(void *at, size_t size) {
			VirtualAlloc(at, size, MEM_COMMIT | commitFlags, allocProt);
		}

		void VMWin::decommit(void *at, size_t size) {
			VirtualFree(at, size, MEM_DECOMMIT);
		}

		void VMWin::free(void *at, size_t size) {
			VirtualFree(at, 0, MEM_RELEASE);
		}

	}
}

#endif
