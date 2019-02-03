#include "stdafx.h"
#include "Generation.h"

#if STORM_GC == STORM_GC_SMM

#include "Arena.h"

namespace storm {
	namespace smm {

		Generation::Generation(Arena &owner, size_t size)
			: totalSize(size), blockSize(size / 2),
			  next(null), owner(owner), sharedBlock(null) {}

		Generation::~Generation() {
			// Free all blocks we are in charge of.
			InlineSet<Block>::iterator i = blocks.begin();
			while (i != blocks.end()) {
				Block *remove = *i;
				++i;
				blocks.erase(remove);
			}
		}

		Block *Generation::alloc() {
			util::Lock::L z(lock);
			return allocBlock(blockSize);
		}

		void Generation::done(Block *block) {
			util::Lock::L z(lock);

			// TODO: Look at the amount of memory we're using, and consider triggering a garbage
			// collection, at least for this generation.
		}

		Block *Generation::fillBlock(size_t size) {
			if (sharedBlock && sharedBlock->remaining() < size) {
				done(sharedBlock);
				sharedBlock = null;
			}

			if (!sharedBlock)
				sharedBlock = allocBlock(max(size, blockSize));

			return sharedBlock;
		}

		Block *Generation::allocBlock(size_t size) {
			Block *b = owner.allocMin(size);
			if (b)
				blocks.insert(b);

			return b;
		}

	}
}

#endif
