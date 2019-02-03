#pragma once

#if STORM_GC == STORM_GC_SMM

#include "VM.h"
#include "Generation.h"

namespace storm {
	namespace smm {

		class Block;

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
			// memory for the arena. Also, create 'generationCount' generations, each with the
			// specified size (in bytes).
			Arena(size_t initialSize, const size_t *generations, size_t generationCount);

			// Destroy.
			~Arena();

			// The generations in use (array).
			Generation *generations;

			// Number of generations.
			const size_t generationCount;

			// Allocate a block with a specified minimum size. The actual size may be larger. 'size'
			// excludes the bytes required for the Block-header.
			Block *allocMin(size_t size);

			// Free a block.
			void free(Block *block);

		private:
			// No copying!
			Arena(const Arena &o);
			Arena &operator =(const Arena &o);

			// VM management.
			VM *vm;
		};

	}
}

#endif
