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
		// TODO: We probably want to increase this a bit, it lowers false positives and thus increases
		// the number of objects we can reclaim in many cases. The cost is fairly negligible
		// (pinnedBytes + sizeof(size_t) bytes for every allocated chunk).
		static const size_t pinnedBytes = sizeof(size_t) * 16;

		// Size of the history stored. A longer history uses more memory, but makes hash tables (or
		// other code that depends on actual pointer values) take the slow path more
		// often. Must be a power of two!
		static const nat historySize = 16;

		// Granularity of memory allocations in the VMAlloc class. For each 2^vmAllocBits bytes
		// managed, one byte of additional memory is needed. For a value of 16, blocks are 64KiB and
		// 32KiB memory is needed for managing 2GiB memory.
		static const size_t vmAllocBits = 16;
		static const size_t vmAllocMinSize = 1 << vmAllocBits;

		// Maximum number of bits for use in generation identifiers. Enforced by VMAlloc.h.
		static const size_t identifierBits = CHAR_BIT - 2;
		static const byte identifierMaxVal = byte(1) << identifierBits;

		// Reserved number of identifiers = first free identifier number.
		static const byte firstFreeIdentifier = 0;

		// Identifier used by the finalizer generation. This is outside of what we can store in a
		// GenSet, but that is fine since it does not count as a generation.
		static const byte finalizerIdentifier = identifierMaxVal - 1;

		// Identifier used by the nonmoving allocations. This is outside of what we can store in a
		// GenSet, but that is fine since nonmoving allocations don't belong to any particular generation.
		static const byte nonmovingIdentifier = identifierMaxVal - 2;

	}
}

#endif
