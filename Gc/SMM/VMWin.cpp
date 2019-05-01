#include "stdafx.h"
#include "VMWin.h"

#if STORM_GC == STORM_GC_SMM && defined(WINDOWS)

#include "VMAlloc.h"

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

		void VMWin::watchWrites(VMAlloc *alloc, void *at, size_t size) {
			// Just reset the state here, so that we don't report writes that may have occurred before this call.
			ResetWriteWatch(at, size);
		}

		void VMWin::notifyWrites(VMAlloc *alloc, void **buffer) {
			const vector<Chunk> &chunks = alloc->chunkList();
			for (size_t i = 0; i < chunks.size(); i++) {
				notifyWrites(alloc, buffer, chunks[i].at, chunks[i].size);
			}
		}

		struct AddrCmp {
			bool operator ()(void *l, void *r) const {
				return size_t(l) < size_t(r);
			}
		};

		void VMWin::notifyWrites(VMAlloc *alloc, void **buffer, void *at, size_t size) {
			// Note: This is fairly expensive...
			ULONG_PTR count = 0;
			DWORD granularity = 0;
			do {
				count = arenaBufferWords;
				UINT r = GetWriteWatch(WRITE_WATCH_FLAG_RESET, at, size, buffer, &count, &granularity);
				// TODO: If GetWriteWatch fails, something is *really* bad. Perhaps we should try to
				// mark all pages in the interval if that happens.
				dbg_assert(r == 0, L"GetWriteWatch failed");

				// Mark the blocks that contain the addresses as updated. We do this in a batch so
				// that the implementation doesn't need to use the lookup table for every address.
				alloc->markBlockWrites(buffer, size_t(count));
			} while (count == arenaBufferWords);
		}

	}
}

#endif
