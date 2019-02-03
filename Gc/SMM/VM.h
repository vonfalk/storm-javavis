#pragma once

#if STORM_GC == STORM_GC_SMM

#include "Utils/Object.h"

namespace storm {
	namespace smm {

		/**
		 * Management of virtual memory.
		 *
		 * There are several implementations available, depending on the current operatins system in
		 * use, and what features are supported.
		 */
		class VM : NoCopy {
		public:
			// Create a VM instance suitable for the current system.
			static VM *create(size_t reservedSize);

			// Page size.
			const size_t pageSize;

			// Current granularity of allocations (made through this interface, not necessarily the
			// granularity provided by the underlying operating system).
			const size_t allocGranularity;

			// Allocate a chunk of memory.
			virtual void *alloc(size_t size) = 0;

			// Free previously allocated memory.
			virtual void free(void *mem, size_t size) = 0;

		protected:
			VM(size_t pageSize, size_t granularity) : pageSize(pageSize), allocGranularity(granularity) {}
		};

	}
}

#endif
