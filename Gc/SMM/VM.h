#pragma once

#if STORM_GC == STORM_GC_SMM

namespace storm {
	namespace smm {

		/**
		 * Platform-specific functions for managing virtual memory.
		 */

		// Get the current pagesize.
		size_t vmPageSize();

		// Get the current allocation granularity (may differ from the page size).
		size_t vmAllocGranularity();

		// Allocate virtual memory, immediately committing it.
		void *vmAlloc(size_t size);

		// Free previously allocated virtual memory.
		void vmFree(void *mem, size_t size);

	}
}

#endif
