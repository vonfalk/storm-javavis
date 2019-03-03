#pragma once

#if STORM_GC == STORM_GC_SMM

#include "Utils/Object.h"

namespace storm {
	namespace smm {

		class BlockAlloc;

		/**
		 * Management of virtual memory.
		 *
		 * There are several implementations available, depending on the current operatins system in
		 * use, and what features are supported.
		 *
		 * Memory can be managed down to the limit specified by 'pageSize', but allocations may
		 * require a larger alignment, specified by 'allocGranularity'. 'allocGranularity' must be a
		 * multiple of 'pageSize'.
		 */
		class VM : NoCopy {
		public:
			// Create a VM instance suitable for the current system.
			static VM *create();

			// Page size.
			const size_t pageSize;

			// Current granularity of allocations (made through this interface, not necessarily the
			// granularity provided by the underlying operating system).
			const size_t allocGranularity;

			// Reserve memory (do not commit the memory yet). If 'at' is null, the implementation
			// chooses where to place the allocation. Otherwise, the implementation either takes the
			// address as a hint and attempts to allocate somewhere nearby, or fails if the desired
			// address (rounded to 'allocGranularity') is not available.
			virtual void *reserve(void *at, size_t size) = 0;

			// Commit some previously reserved memory.
			virtual void commit(void *at, size_t size) = 0;

			// Decommit some committed memory.
			virtual void decommit(void *at, size_t size) = 0;

			// Free reserved and/or committed memory. Calls to 'free' need to correspond to a call
			// made previously by 'reserve'. I.e. it is not possible to free parts of previously
			// reserved memory.
			virtual void free(void *at, size_t size) = 0;

			// Arrange so that any writes to the indicated addresses will result in the 'fUpdated'
			// flag for the corresponding block to be set. After the 'fUpdated' flag has been set,
			// the pages need to be registered once more if further write notifications are to be
			// sent. It is assumed that the indicated addresses do not span more than one block.
			virtual void watchWrites(BlockAlloc *alloc, void *at, size_t size) = 0;

			// Called by the system to tell the VM subsystem to check for any writes that needs to
			// be notified. Some implementations require calls to this function for 'watchWrites' to
			// work properly. "buffer" is assumed to be at least "arenaBuffer" words long.
			virtual void notifyWrites(BlockAlloc *alloc, void **buffer) = 0;

		protected:
			VM(size_t pageSize, size_t granularity) : pageSize(pageSize), allocGranularity(granularity) {}
		};

	}
}

#endif
