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
			static VMWin *create(size_t initialSize);

			// Allocate memory.
			virtual void *alloc(size_t size);

			// Free memory.
			virtual void free(void *mem, size_t size);

		private:
			VMWin(size_t pageSize, size_t granularity);
		};

	}
}

#endif
