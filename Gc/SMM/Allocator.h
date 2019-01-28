#pragma once

#if STORM_GC == STORM_GC_SMM

namespace storm {
	namespace smm {

		/**
		 * An allocator is the interface the rest of the system uses to request memory from the GC
		 * in units smaller than whole blocks.
		 *
		 * Each allocator instance have their own Block (nursery generation) used for allocations,
		 * so that the majority of allocations can be performed without acquiring any locks.
		 */
		class Allocator {
		public:

		private:
			// No copying!
			Allocator(const Allocator &o);
			Allocator &operator =(const Allocator &o);
		};

	}
}

#endif
