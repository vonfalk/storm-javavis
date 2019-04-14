#include "stdafx.h"
#include "VMPosix.h"

#if STORM_GC == STORM_GC_SMM && defined(POSIX)

#include "VMAlloc.h"
#include <sys/mman.h>

namespace storm {
	namespace smm {

		static const int protection = PROT_EXEC | PROT_READ | PROT_WRITE;
		static const int flags = MAP_ANONYMOUS | MAP_PRIVATE;

		VMPosix *VMPosix::create() {
			size_t pageSize = sysconf(_SC_PAGESIZE);
			return new VMPosix(pageSize);
		}

		VMPosix::VMPosix(size_t pageSize) : VM(pageSize, pageSize) {}

		void *VMPosix::reserve(void *at, size_t size) {
			return mmap(at, size, PROT_NONE, flags, -1, 0);
		}

		void VMPosix::commit(void *at, size_t size) {
			mprotect(at, size, protection);
		}

		void VMPosix::decommit(void *at, size_t size) {
			// Re-map pages with PROT_NONE to tell the kernel we're no longer interested in this memory.
			void *result = mmap(at, size, PROT_NONE, flags | MAP_FIXED, -1, 0);
			if (result == MAP_FAILED)
				PLN(L"Decommit failed!");
		}

		void VMPosix::free(void *at, size_t size) {
			munmap(at, size);
		}

		void VMPosix::watchWrites(VMAlloc *alloc, void *at, size_t size) {
			assert(false, L"watchWrites is not implemented yet!");

			// In order for scanning to work properly, we mark the block as 'updated' immediately.
			Block *b = alloc->findBlock(at);
			if (b)
				b->flags |= Block::fUpdated;
		}

		void notifyWrites(VMAlloc *alloc, void **buffer) {
			// Not needed on Posix.
		}

	}
}

#endif
