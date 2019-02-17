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

	}
}

#endif
