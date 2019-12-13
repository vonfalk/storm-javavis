#pragma once

#if STORM_GC == STORM_GC_SMM && defined(POSIX)

#include "VM.h"
#include <signal.h>

namespace storm {
	namespace smm {

		/**
		 * POSIX-specific implementation of the VM manager.
		 */
		class VMPosix : public VM {
		public:
			// Create.
			static VMPosix *create(VMAlloc *alloc);

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
			virtual void stopWatchWrites(void *at, size_t size);

		private:
			VMPosix(VMAlloc *alloc, size_t pageSize);

			// Handle segmentation faults.
			static void sigsegv(int signal, siginfo_t *info, void *context);
		};

	}
}

#endif
