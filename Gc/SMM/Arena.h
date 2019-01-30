#pragma once

#if STORM_GC == STORM_GC_SMM

namespace storm {
	namespace smm {

		/**
		 * An Arena keeps track of all allocations made by the GC, and represents the entire world
		 * the GC lives in. The first Arena instance will set up signal- or exception handlers as
		 * necessary in order to get notified by the operating system about important memory events.
		 *
		 * An arena is always backed by virtual memory provided by the operating system.
		 *
		 * The arena also keeps track of a set of mutator threads that may access the memory
		 * allocated inside the arena. Other threads accessing the memory may cause unintended
		 * results.
		 */
		class Arena {
		public:
			// Create the arena, initially try to reserve (but not commit) 'initialSize' bytes of
			// memory for the arena.
			Arena(size_t initialSize);

			// Destroy.
			~Arena();

		private:
			// No copying!
			Arena(const Arena &o);
			Arena &operator =(const Arena &o);

			// Granularity of allocations (sometimes not equal to the pagesize).
			size_t allocGranularity;

			// Current pagesize. This is the granularity with which we can affect memory protection
			// in an allocation.
			size_t pageSize;

			// Platform-specific initialization.
			void platformInit();
		};

	}
}

#endif
