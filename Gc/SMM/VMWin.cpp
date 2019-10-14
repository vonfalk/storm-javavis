#include "stdafx.h"
#include "VMWin.h"

#if STORM_GC == STORM_GC_SMM && defined(WINDOWS)

#include "VMAlloc.h"

namespace storm {
	namespace smm {

		// Flags that applies to all allocations.
		static const DWORD reserveFlags = 0;
		static const DWORD commitFlags = 0;
		static const DWORD allocProt = PAGE_EXECUTE_READWRITE;
		static const DWORD wpProt = PAGE_EXECUTE_READ;

		static PVOID handlerHandle = 0;

		VMWin *VMWin::create(VMAlloc *alloc) {
			SYSTEM_INFO info;
			GetSystemInfo(&info);

			return new VMWin(alloc, info.dwPageSize, info.dwAllocationGranularity);
		}

		void VMWin::initNotify() {
			handlerHandle = AddVectoredExceptionHandler(1, &VMWin::onException);
		}

		void VMWin::destroyNotify() {
			RemoveVectoredExceptionHandler(handlerHandle);
		}

		LONG VMWin::onException(struct _EXCEPTION_POINTERS *info) {
			EXCEPTION_RECORD *record = info->ExceptionRecord;

			if (record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
				ULONG_PTR mode = record->ExceptionInformation[0];
				ULONG_PTR addr = record->ExceptionInformation[1];

				// Is it a write?
				if (mode == 1) {
					if (VM::notifyWrite((void *)addr)) {
						return EXCEPTION_CONTINUE_EXECUTION;
					}
				}
			}

			// Fall through to continue searching for all unknown exceptions.
			return EXCEPTION_CONTINUE_SEARCH;
		}

		VMWin::VMWin(VMAlloc *alloc, size_t pageSize, size_t granularity) : VM(alloc, pageSize, granularity) {}

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

		void VMWin::watchWrites(void *at, size_t size) {
			DWORD old;
			VirtualProtect(at, size, wpProt, &old);
		}

		void VMWin::stopWatchWrites(void *at, size_t size) {
			DWORD old;
			VirtualProtect(at, size, allocProt, &old);
		}


	}
}

#endif
