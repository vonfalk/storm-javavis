#pragma once

#if STORM_GC == STORM_GC_SMM && defined(POSIX)

#include "VM.h"

namespace storm {
	namespace smm {

		/**
		 * POSIX-specific implementation of the VM manager.
		 */
		class VMPosix : public VM {
		public:
			// Create.
			static VMPosix *create();

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

			// Notify writes.
			virtual void notifyWrites(VMAlloc *alloc, void **buffer);

		private:
			VMPosix(size_t pageSize);
		};

	}
}

#endif
