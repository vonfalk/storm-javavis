#pragma once

#if STORM_GC == STORM_GC_SMM

#include "Utils/Lock.h"
#include "Block.h"
#include "InlineSet.h"
#include "ArenaEntry.h"

namespace storm {
	namespace smm {

		class Arena;

		/**
		 * A generation is a set of blocks that belong together, and that will be collected
		 * together. A generation additionaly contains information on the typical size of the
		 * contained blocks, and other useful information.
		 */
		class Generation {
		public:
			// Create a generation, and provide an approximate size of the generation.
			Generation(Arena &owner, size_t size);

			// Destroy.
			~Generation();

			// Lock for this generation.
			util::Lock lock;

			// The next generation in the chain. May be null.
			Generation *next;

			// The size of this generation. We strive to keep below this size, but it may
			// occasionally be broken.
			size_t totalSize;

			// The default size of newly allocated blocks in this generation.
			size_t blockSize;

			// Allocate a new block in this generation. When the block is full, it should be
			// finished by calling 'done'. The size of the returned block is at least 'blockSize'.
			Block *alloc();

			// Notify the generation that a block is full and will no longer be used by an
			// allocator.
			void done(Block *block);

			// Access a shared block that is considered to be at the "end of the generation", where
			// anyone may add objects to this generation. When using this block, the lock needs to
			// be held while the filling is in progress. The parameter indicates how much free
			// memory is needed in the returned block.
			Block *fillBlock(size_t freeBytes);

			// Collect garbage in this generation (API will likely change).
			void collect(ArenaEntry &entry);

		private:
			// No copy.
			Generation(const Generation &o);
			Generation &operator =(const Generation &o);

			// Owning arena.
			Arena &owner;

			// All blocks in this generation.
			InlineSet<Block> blocks;

			// The last block, currently being filled. May be null.
			Block *sharedBlock;

			// Allocate blocks with a specific size. For internal use.
			Block *allocBlock(size_t size);
		};

	}
}

#endif