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
			static VMWin *create(VMAlloc *alloc);

			// Initialize/destroy write notification mechanisms.
			static void initNotify();
			static void destroyNotify();

			// Reserve.
			virtual void *reserve(void *at, size_t size);

			// Commit.
			virtual void commit(void *at, size_t size);

			// Decommit.
			virtual void decommit(void *at, size_t size);

			// Free.
			virtual void free(void *at, size_t size);

			// Watch for writes.
			virtual void watchWrites(void *at, size_t size);

			// Stop watching.
			virtual void stopWriteWatch(void *at, size_t size);

		private:
			VMWin(VMAlloc *alloc, size_t pageSize, size_t granularity);

			static LONG WINAPI onException(struct _EXCEPTION_POINTERS *info);
		};

	}
}

#endif
