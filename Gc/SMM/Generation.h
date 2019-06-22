#pragma once

#if STORM_GC == STORM_GC_SMM

#include "Utils/Lock.h"
#include "Block.h"
#include "InlineSet.h"
#include "Arena.h"
#include "Predicates.h"

namespace storm {
	namespace smm {

		class ScanState;

		/**
		 * A generation is a set of blocks that belong together, and that will be collected
		 * together. A generation additionaly contains information on the typical size of the
		 * contained blocks, and other useful information.
		 *
		 * Note that it is required to acquire the GC lock before interfacing with this class.
		 */
		class Generation {
		public:
			// Create a generation, and provide an approximate size of the generation.
			Generation(Arena &arena, size_t size, byte identifier);

			// Destroy.
			~Generation();

			// The next generation in the chain. May be null.
			Generation *next;

			// Owning arena.
			Arena &arena;

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
			Block *alloc(ArenaTicket &ticket, size_t minSize);

			// Notify the generation that a block is full and will no longer be used by an
			// allocator.
			void done(ArenaTicket &ticket, Block *block);

			// Lock for accessing the shared block returned by 'sharedBlock'.
			util::Lock sharedBlockLock;

			// Access a shared block that is considered to be at the "end of the generation", where
			// anyone may add objects to this generation. When using this block, the lock needs to
			// be held while the filling is in progress. The parameter indicates how much free
			// memory is needed in the returned block.
			Block *sharedBlock(ArenaTicket &ticket, size_t freeBytes);

			// Perform a full collection of this generation. We probably want a more fine-grained
			// API in the future.
			void collect(ArenaTicket &ticket);

			// Scan all blocks in this generation with the specified scanner.
			template <class Scanner>
			typename Scanner::Result scan(typename Scanner::Source &source);

			template <class Predicate, class Scanner>
			typename Scanner::Result scan(const Predicate &predicate, typename Scanner::Source &source);

			// Scan all blocks that may contain references to anything in the provided GenSet.
			template <class Scanner>
			typename Scanner::Result scan(GenSet refsTo, typename Scanner::Source &source);

			template <class Predicate, class Scanner>
			typename Scanner::Result scan(const Predicate &predicate, GenSet refsTo, typename Scanner::Source &source);

			// Run all finalizers in this block. Most likely only called before the entire Arena is
			// destroyed, so no need for efficiency.
			void runAllFinalizers();


			/**
			 * Class that is handed out to give additional information during a collection.
			 *
			 * Instances of this class are only handed out during an ongoing scan, and since it can
			 * not be copied, it ensures that it is not possible to call 'isPinned' if there is no
			 * data to return from 'isPinned'.
			 */
			class State {
				friend class Generation;
			public:
				// The generation itself.
				Generation &gen;

				// Get the generation id.
				inline byte identifier() const {
					return gen.identifier;
				}

				// Get information on pinned objects in this generation.
				inline bool isPinned(void *obj, void *end) const {
					return gen.isPinned(obj, end);
				}

				// Get the arena.
				inline Arena &arena() const {
					return gen.arena;
				}

			private:
				State(Generation &gen) : gen(gen) {}
				State(State &o);
			};

			// Fill a memory summary with information.
			void fillSummary(MemorySummary &summary) const;

			// Verify the integrity of all blocks in this generation.
			void dbg_verify();

			// Output a summary.
			void dbg_dump();

		private:

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

				template <class Predicate, class Scanner>
				typename Scanner::Result scan(const Predicate &predicate, typename Scanner::Source &source);

				// Scan blocks which may refer to objects in the specified generations.
				template <class Scanner>
				typename Scanner::Result scan(GenSet toScan, typename Scanner::Source &source);

				template <class Predicate, class Scanner>
				typename Scanner::Result scan(const Predicate &p, GenSet toScan, typename Scanner::Source &source);

				// Scan all pinned objects in this chunk.
				template <class Scanner>
				typename Scanner::Result scanPinned(const PinnedSet &pinned,
													typename Scanner::Source &source);

				template <class Predicate, class Scanner>
				typename Scanner::Result scanPinned(const Predicate &predicate,
													const PinnedSet &pinned,
													typename Scanner::Source &source);

				// Scan pinned objects in this chunk, also collecting all blocks marked as
				// containing finalizers in a big list.
				template <class Scanner>
				typename Scanner::Result scanPinnedFindFinalizers(const PinnedSet &pinned,
																typename Scanner::Source &source,
																Block *&finalizers);

				template <class Predicate, class Scanner>
				typename Scanner::Result scanPinnedFindFinalizers(const Predicate &predicate,
																const PinnedSet &pinned,
																typename Scanner::Source &source,
																Block *&finalizers);

				// Run all finalizers in this chunk.
				void runAllFinalizers();

				// Memory summary.
				void fillSummary(MemorySummary &summary) const;

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
				// created block header is returned, as an update to 'last'. Takes care of objects
				// in need of finalization.
				Block *compactPinned(Block *last, Block *block, const PinnedSet &pinned);

				// Shrink a block as much as possible while keeping pinned objects intact. Replaces
				// non-pinned objects with padding. Takes care of objects in need of finalization.
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
			// Note: We use the last element here to keep track of shared objects!
			vector<PinnedSet> pinnedSets;

			// Get the minimum size we want our blocks to be when we're splitting them.
			inline size_t minFragment() const { return blockSize >> 2; }

			// Last chunk examined for free memory. The next allocation will continue from here.
			size_t lastChunk;

			// Total number of allocated bytes in all chunks.
			size_t totalAllocBytes;

			// Total number of free bytes in all chunks.
			size_t totalFreeBytes;

			// Total number of used bytes.
			inline size_t totalUsedBytes() const { return totalAllocBytes - totalFreeBytes; }

			// Chunk size requested from the underlying system by default.
			// TODO: What is good here?
			inline size_t defaultChunkSize() const { return blockSize * 32; }

			// The shared block.
			Block *shared;

			// Allocate a block with a (usable) size in the specified range. For internal use.
			Block *allocBlock(ArenaTicket &ticket, size_t minSize, size_t maxSize);

			// Find a suitable block to allocate. Never attempts to allocate more memory from the
			// Arena. Use 'allocBlock' for that.
			Block *findFreeBlock(size_t minSize, size_t maxSize);

			// Check if a particular object is pinned. Only reasonable to call during an ongoing scan.
			bool isPinned(void *obj, void *end);
		};


		template <class Scanner>
		typename Scanner::Result Generation::scan(typename Scanner::Source &source) {
			return scan<fmt::ScanAll, Scanner>(fmt::ScanAll(), source);
		}

		template <class Predicate, class Scanner>
		typename Scanner::Result Generation::scan(const Predicate &predicate, typename Scanner::Source &source) {
			typename Scanner::Result r = typename Scanner::Result();
			for (size_t i = 0; i < chunks.size(); i++) {
				r = chunks[i].scan<Predicate, Scanner>(predicate, source);
				if (r != typename Scanner::Result())
					return r;
			}
			return r;
		}

		template <class Scanner>
		typename Scanner::Result Generation::scan(GenSet toScan, typename Scanner::Source &source) {
			return scan<fmt::ScanAll, Scanner>(fmt::ScanAll(), toScan, source);
		}

		template <class Predicate, class Scanner>
		typename Scanner::Result Generation::scan(const Predicate &p, GenSet toScan, typename Scanner::Source &source) {
			typename Scanner::Result r = typename Scanner::Result();
			for (size_t i = 0; i < chunks.size(); i++) {
				chunks[i].scan<Predicate, Scanner>(p, toScan, source);
				if (r != typename Scanner::Result())
					return r;
			}
			return r;
		}


		template <class Scanner>
		typename Scanner::Result Generation::GenChunk::scan(typename Scanner::Source &source) {
			return scan<fmt::ScanAll, Scanner>(fmt::ScanAll(), source);
		}

		template <class Predicate, class Scanner>
		typename Scanner::Result Generation::GenChunk::scan(const Predicate &p, typename Scanner::Source &source) {
			typename Scanner::Result r = typename Scanner::Result();
			for (Block *at = (Block *)memory.at; at != (Block *)memory.end(); at = (Block *)at->mem(at->size)) {
				if (at->hasFlag(Block::fSkipScan)) {
					at->clearFlag(Block::fSkipScan);
					continue;
				}

				r = at->scanIf<Predicate, Scanner>(p, source);
				if (r != typename Scanner::Result())
					return r;
			}
			return r;
		}

		template <class Scanner>
		typename Scanner::Result Generation::GenChunk::scan(GenSet toScan, typename Scanner::Source &source) {
			return scan<fmt::ScanAll, Scanner>(fmt::ScanAll(), toScan, source);
		}

		template <class Predicate, class Scanner>
		typename Scanner::Result Generation::GenChunk::scan(const Predicate &predicate,
															GenSet toScan,
															typename Scanner::Source &source) {
			// TODO: It would be useful to have some kind of 'early out' here!

			typename Scanner::Result r = typename Scanner::Result();
			for (Block *at = (Block *)memory.at; at != (Block *)memory.end(); at = (Block *)at->mem(at->size)) {
				if (at->hasFlag(Block::fSkipScan)) {
					at->clearFlag(Block::fSkipScan);
					continue;
				}

				if (!at->mayReferTo(toScan))
					continue;

				r = at->scanIf<Predicate, Scanner>(predicate, source);
				if (r != typename Scanner::Result())
					return r;
			}
			return r;
		}

		template <class Scanner>
		typename Scanner::Result Generation::GenChunk::scanPinned(const PinnedSet &pinned,
																typename Scanner::Source &source) {
			return scanPinned<fmt::ScanAll, Scanner>(fmt::ScanAll(), pinned, source);
		}

		template <class Predicate, class Scanner>
		typename Scanner::Result Generation::GenChunk::scanPinned(const Predicate &predicate,
																const PinnedSet &pinned,
																typename Scanner::Source &source) {

			typename Scanner::Result r = typename Scanner::Result();
			if (pinned.empty())
				return r;

			typedef IfBoth<IfPinned, Predicate> P;
			P p(pinned, predicate);

			// Walk all blocks and scan the relevant ones.
			for (Block *at = (Block *)memory.at; at != (Block *)memory.end(); at = (Block *)at->mem(at->size)) {
				if (pinned.has(at->mem(0), at->mem(at->committed()))) {
					r = at->scanIf<P, Scanner>(p, source);
					if (r != typename Scanner::Result())
						return r;
				}
			}

			return r;
		}

		template <class Scanner>
		typename Scanner::Result Generation::GenChunk::scanPinnedFindFinalizers(const PinnedSet &pinned,
																				typename Scanner::Source &source,
																				Block *&finalizers) {
			return scanPinnedFindFinalizers<fmt::ScanAll, Scanner>(fmt::ScanAll(), pinned, source, finalizers);
		}

		template <class Predicate, class Scanner>
		typename Scanner::Result Generation::GenChunk::scanPinnedFindFinalizers(const Predicate &predicate,
																				const PinnedSet &pinned,
																				typename Scanner::Source &source,
																				Block *&finalizers) {

			typename Scanner::Result r = typename Scanner::Result();

			typedef IfBoth<IfPinned, Predicate> P;
			P p(pinned, predicate);

			// Walk all blocks and scan the relevant ones.
			for (Block *at = (Block *)memory.at; at != (Block *)memory.end(); at = (Block *)at->mem(at->size)) {
				if (pinned.has(at->mem(0), at->mem(at->committed()))) {
					r = at->scanIf<P, Scanner>(p, source);
					if (r != typename Scanner::Result())
						return r;
				}

				if (at->hasFlag(Block::fFinalizers)) {
					// We're going to trash 'reserved' anyway, so we don't need any extra padding to use 'next'.
					at->next(finalizers);
					finalizers = at;
				}
			}

			return r;
		}

	}
}

#endif
