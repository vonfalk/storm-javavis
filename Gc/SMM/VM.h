#pragma once

#if STORM_GC == STORM_GC_SMM

#include "Utils/Object.h"

namespace storm {
	namespace smm {

		class VMAlloc;

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
			virtual ~VM();

			// Create a VM instance suitable for the current system. 'alloc' will be notified of any
			// writes that occur to write protected pages.
			static VM *create(VMAlloc *alloc);

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
			virtual void watchWrites(void *at, size_t size) = 0;

			// Remove the write notification for all pages in the specified range.
			virtual void stopWriteWatch(void *at, size_t size) = 0;

		protected:
			VM(VMAlloc *alloc, size_t pageSize, size_t granularity);

			// The VMAlloc instance to be notified of writes.
			VMAlloc *notify;

			// VMAlloc objects registered at a global level.
			static VMAlloc **globalArenas;

			// Any 'notifyWrite' instances currently using 'globalArenas'?
			static size_t usingGlobal;

			// Lock used when modifying 'globalArenas'.
			static util::Lock globalLock;

			// Notify the proper VMAlloc instance of a detected write. Returns 'true' if we found
			// someone that handled the write notification.
			static bool notifyWrite(void *addr);

			// Initialize write notifications for the current backend.
			static void initNotify();
			static void destroyNotify();
		};

	}
}

#endif
