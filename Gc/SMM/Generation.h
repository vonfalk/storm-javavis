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
			Generation(Arena &owner, size_t size, byte identifier);

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
			// finished by calling 'done'. The size of the returned block has at least 'minSize'
			// free memory. Allocations where 'minSize' is much larger than 'blockSize' may not be
			// fulfilled.
			Block *alloc(size_t minSize);

			// Notify the generation that a block is full and will no longer be used by an
			// allocator.
			void done(Block *block);

			// Access a shared block that is considered to be at the "end of the generation", where
			// anyone may add objects to this generation. When using this block, the lock needs to
			// be held while the filling is in progress. The parameter indicates how much free
			// memory is needed in the returned block.
			Block *fillBlock(size_t freeBytes);

		private:
			// No copy.
			Generation(const Generation &o);
			Generation &operator =(const Generation &o);

			// Owning arena.
			Arena &owner;

			// Our identifier.
			byte identifier;

			struct GenChunk {
				// Create. Initializes the chunk to contain a single empty block.
				GenChunk(Chunk chunk);

				// The entire chunk.
				Chunk memory;

				// Pointer to the block that was allocated last. Empty memory is likely just after this block!
				Block *lastAlloc;

				// Number of non-allocated bytes in this chunk. Used to quickly determine where to
				// look for free space when allocating memory. This does *not* include bytes in
				// chunks marked as 'used'.
				size_t freeBytes;

				// Allocate a block inside this chunk. Returns null on failure. 'minFragment' states
				// how small fragments that are acceptable when splitting blocks. Marks the returned
				// block as 'in use'. Call 'releaseBlock' to remove the mark.
				Block *allocBlock(size_t minSize, size_t maxSize, size_t minFragment);

				// Mark the block as no longer in use, allowing re-use of any remaining memory inside.
				void releaseBlock(Block *block);
			};

			struct ChunkCompare {
				inline bool operator ()(const GenChunk &a, const GenChunk &b) const {
					return size_t(a.memory.at) < size_t(b.memory.at);
				}
				inline bool operator ()(const GenChunk &a, const Chunk &b) const {
					return size_t(a.memory.at) < size_t(b.at);
				}
			};

			// Note: Compares with the *end* of chunks, so that lower_bound returns the interesting
			// element directly, rather than us having to step back one step each time.
			struct PtrCompare {
				inline bool operator ()(const GenChunk &a, const void *ptr) const {
					return size_t(a.memory.at) + a.memory.size < size_t(ptr);
				}
			};

			// Chunks of memory allocated from the virtual memory manager. Each of these chunks
			// contain one or more Blocks, back to back. Blocks are split and merged as necessary to
			// provide blocks of suitable sizes from the 'alloc' functions. All blocks that have
			// been handed out through 'alloc' have the fUsed flag set.
			// TODO: Is it useful to use an InlineSet here rather than a vector that uses the default allocator?
			// with an InlineSet, we could allocate our metadata at the beginning of each underlying allocation
			// and link them together.
			// Elements here are sorted according to their address.
			typedef vector<GenChunk> ChunkList;
			ChunkList chunks;

			// Get the minimum size we want our blocks to be when we're splitting them.
			inline size_t minFragment() const { return blockSize >> 2; }

			// Last chunk examined for free memory. The next allocation will continue from here.
			size_t lastChunk;

			// The last block, currently being filled. May be null.
			Block *sharedBlock;

			// Allocate a block with a (usable) size in the specified range. For internal use.
			Block *allocBlock(size_t minSize, size_t maxSize);
		};

	}
}

#endif
