#pragma once

#if STORM_GC == STORM_GC_SMM && defined(WINDOWS)

#include "VM.h"

namespace storm {
	namespace smm {

		/**
		 * Windows-specific implementation of the VM manager.
		 */
		class VMWin : public VM {
		public:
			// Create.
			static VMWin *create();

			// Reserve.
			virtual void *reserve(void *at, size_t size);

			// Commit.
			virtual void commit(void *at, size_t size);

			// Decommit.
			virtual void decommit(void *at, size_t size);

			// Free.
			virtual void free(void *at, size_t size);

			// Watch for writes.
			virtual void watchWrites(VMAlloc *alloc, void *at, size_t size);

			// Check for writes.
			virtual void notifyWrites(VMAlloc *alloc, void **buffer);

		private:
			VMWin(size_t pageSize, size_t granularity);

			// Check a number of pages for writes.
			void notifyWrites(VMAlloc *alloc, void **buffer, void *at, size_t size);
		};

	}
}

#endif
