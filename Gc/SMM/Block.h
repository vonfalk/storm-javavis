#pragma once

#if STORM_GC == STORM_GC_SMM

#include "InlineSet.h"

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
		class Block : public SetMember<Block> {
		public:
			Block(size_t size) : size(size), committed(0), reserved(0) {}

			// Current size (excluding the block itself).
			const size_t size;

			// Amount of memory committed (excluding the block itself).
			size_t committed;

			// Amount of memory reserved. 'reserved >= committed'. Memory that is reserved but not
			// committed is being initialized, and can not be assumed to contain usable data.
			size_t reserved;

			// Get the number of bytes remaining in this block (ignoring any reserved memory).
			inline size_t remaining() const {
				return size - committed;
			}

			// Get a pointer to a particular byte of the memory in this block.
			inline void *mem(size_t offset) {
				void *ptr = this;
				return (byte *)ptr + sizeof(Block) + offset;
			}

		private:
			// No copying!
			Block(const Block &o);
			Block &operator =(const Block &o);
		};

	}
}

#endif
