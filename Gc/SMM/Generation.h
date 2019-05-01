#pragma once

#if STORM_GC == STORM_GC_SMM

#include "Utils/Lock.h"
#include "Block.h"
#include "InlineSet.h"
#include "ArenaEntry.h"

namespace storm {
	namespace smm {

		class Arena;
		class ScanState;

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

			// Owning arena.
			Arena &owner;

			// Our identifier.
			const byte identifier;

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

			// Perform a full collection of this generation. We probably want a more fine-grained
			// API in the future.
			void collect(ArenaEntry &entry);

			// Scan all blocks in this generation with the specified scanner.
			template <class Scanner>
			typename Scanner::Result scan(typename Scanner::Source &source);

			// Scan all blocks that may contain references to anything in the provided GenSet.
			template <class Scanner>
			typename Scanner::Result scan(GenSet refsTo, typename Scanner::Source &source);

			// Verify the integrity of all blocks in this generation.
			void dbg_verify();

			// Output a summary.
			void dbg_dump();

		private:
			friend class ScanState;

			// No copy.
			Generation(const Generation &o);
			Generation &operator =(const Generation &o);

			/**
			 * A chunk managed by a generation. Perhaps this should be moved outside of the generation class.
			 *
			 * TODO: Since the underlying memory manager does not necessarily have a granularity
			 * better than these chunks, perhaps we should move summaries to these chunks rather
			 * than keeping them inside the blocks.
			 */
			class GenChunk {
			public:
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

				// Compact the blocks in this chunk, making sure to preserve the location of any
				// pinned objects. All non-pinned objects are assumed to be collectable. Returns
				// 'true' if the entire block is empty after compaction. Compaction will re-set the
				// 'reserved' parts of all remaining blocks to indicate that allocations have to be
				// re-tried. Blocks marked as 'used' are not re-allocated as other parts of the
				// system may have pointers to them. They will, however, be emptied if possible.
				bool compact(const PinnedSet &pinned);

				// Scan object using the selected scanner.
				template <class Scanner>
				typename Scanner::Result scan(typename Scanner::Source &source);

				// Scan blocks which may refer to objects in the specified generations.
				template <class Scanner>
				typename Scanner::Result scan(GenSet toScan, typename Scanner::Source &source);

				// Scan all pinned objects in this chunk.
				template <class Scanner>
				typename Scanner::Result scanPinned(const PinnedSet &pinned, typename Scanner::Source &source);

				// Verify this chunk.
				void dbg_verify();

				// Output a summary.
				void dbg_dump();

			private:
				// Called when 'until' has been determined as in use by an object during
				// compaction. Updates the header of the previous block and returns a header to the
				// new current header.
				Block *compactFinishObj(Block *current, void *until);

				// Similar to 'compactFinish', but 'until' is known to be a valid block header. As
				// such, this function does not need to create a new block header, but rather reuses
				// the one already present in 'next'.
				void compactFinishBlock(Block *current, Block *next);

				// Compact an unused block, 'block', keeping pinned objects intact. 'last' is the
				// most recent block header that has been determined to remain. The most recently
				// created block header is returned, as an update to 'last'.
				Block *compactPinned(Block *last, Block *block, const PinnedSet &pinned);

				// Shrink a block as much as possible while keeping pinned objects intact. Replaces
				// non-pinned objects with padding.
				void shrinkBlock(Block *block, const PinnedSet &pinned);

				// Predicate that checks if a particular object is pinned.
				struct IfPinned {
					IfPinned(const PinnedSet &pinned) : pinned(pinned) {}

					PinnedSet pinned;

					bool operator() (void *from, void *to) const {
						return pinned.has(from, to);
					}
				};
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

			// Pre-allocated vector of pointer summaries for each block. Used when scanning inexact
			// roots, and due to the heavy usage it is useful to keep them together. The individual
			// elements are neither initialized nor kept up to date outside the scanning functions.
			vector<PinnedSet> pinnedSets;

			// Get the minimum size we want our blocks to be when we're splitting them.
			inline size_t minFragment() const { return blockSize >> 2; }

			// Last chunk examined for free memory. The next allocation will continue from here.
			size_t lastChunk;

			// The last block, currently being filled. May be null.
			Block *sharedBlock;

			// Allocate a block with a (usable) size in the specified range. For internal use.
			Block *allocBlock(size_t minSize, size_t maxSize);

			// Check if a particular object is pinned. Only reasonable to call during an ongoing scan.
			bool isPinned(void *obj, void *end);
		};


		template <class Scanner>
		typename Scanner::Result Generation::scan(typename Scanner::Source &source) {
			typename Scanner::Result r = typename Scanner::Result();
			for (size_t i = 0; i < chunks.size(); i++) {
				r = chunks[i].scan<Scanner>(source);
				if (r != typename Scanner::Result())
					return r;
			}
			return r;
		}

		template <class Scanner>
		typename Scanner::Result Generation::scan(GenSet toScan, typename Scanner::Source &source) {
			typename Scanner::Result r = typename Scanner::Result();
			for (size_t i = 0; i < chunks.size(); i++) {
				chunks[i].scan<Scanner>(toScan, source);
				if (r != typename Scanner::Result())
					return r;
			}
			return r;
		}


		template <class Scanner>
		typename Scanner::Result Generation::GenChunk::scan(typename Scanner::Source &source) {
			typename Scanner::Result r = typename Scanner::Result();
			for (Block *at = (Block *)memory.at; at != (Block *)memory.end(); at = (Block *)at->mem(at->size)) {
				r = at->scan<Scanner>(source);
				if (r != typename Scanner::Result())
					return r;
			}
			return r;
		}

		template <class Scanner>
		typename Scanner::Result Generation::GenChunk::scan(GenSet toScan, typename Scanner::Source &source) {
			// TODO: It would be useful to have some kind of 'early out' here!

			typename Scanner::Result r = typename Scanner::Result();
			for (Block *at = (Block *)memory.at; at != (Block *)memory.end(); at = (Block *)at->mem(at->size)) {
				if (!at->mayReferTo(toScan))
					continue;

				r = at->scan<Scanner>(source);
				if (r != typename Scanner::Result())
					return r;
			}
			return r;
		}

		template <class Scanner>
		typename Scanner::Result Generation::GenChunk::scanPinned(const PinnedSet &pinned,
																typename Scanner::Source &source) {

			typename Scanner::Result r = typename Scanner::Result();
			if (pinned.empty())
				return r;

			// Walk all blocks and scan the relevant ones.
			for (Block *at = (Block *)memory.at; at != (Block *)memory.end(); at = (Block *)at->mem(at->size)) {
				if (pinned.has(at->mem(0), at->mem(at->committed()))) {
					r = at->scanIf<IfPinned, Scanner>(pinned, source);
					if (r != typename Scanner::Result())
						return r;
				}
			}

			return r;
		}

	}
}

#endif
