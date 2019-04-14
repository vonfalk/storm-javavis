#pragma once

#if STORM_GC == STORM_GC_SMM

namespace storm {
	namespace smm {

		/**
		 * Configuration of various constants in the SMM GC. These can be used to tune the
		 * performance, at least to some extent.
		 */

		// Number of bytes used to represent pointer summaries in the GC and in the block allocator.
		static const size_t summaryBytes = sizeof(size_t) * 2;

		// Number of bytes used to represent a set of pointers to pinned objects.
		static const size_t pinnedBytes = sizeof(size_t) * 2;

		// Size of the shared buffer in the Arena instance that is used by various parts of the system.
		// Note: Unit is # of pointers, not bytes.
		static const size_t arenaBufferWords = 128;

		// Granularity of memory allocations in the VMAlloc class. For each 2^vmAllocBits bytes
		// managed, one byte of additional memory is needed. For a value of 16, blocks are 64KiB and
		// 32KiB memory is needed for managing 2GiB memory.
		static const size_t vmAllocBits = 16;
		static const size_t vmAllocMinSize = 1 << vmAllocBits;

	}
}

#endif
