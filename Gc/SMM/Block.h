#pragma once

#if STORM_GC == STORM_GC_SMM
#include "Arena.h"

namespace storm {
	namespace smm {

		/**
		 * A block of memory allocated by an Arena, with some associated metadata required by the
		 * collector. Blocks are generally allocated as a large chunk, where the Block instance is
		 * in the beginning of the allocation, and the remainder of the allocation is the actual
		 * memory.
		 *
		 * Blocks can be considered to be garbage collected, as they are removed (or recycled) when
		 * all objects inside them have been removed.
		 *
		 * We assume memory inside the blocks are allocated from low addresses to high addressed
		 * simply by bumping a pointer, and the Block keeps track of this. Blocks are never moved,
		 * even though objects inside a block may move inside or between blocks.
		 */
		class Block {
		public:
			Block(size_t size) : size(size), filled(0) {}

			// Current size (excluding the block itself).
			size_t size;

			// Amount of memory used (excluding the block itself).
			size_t filled;

		private:
			// No copying!
			Block(const Block &o);
			Block &operator =(const Block &o);
		};

	}
}

#endif
