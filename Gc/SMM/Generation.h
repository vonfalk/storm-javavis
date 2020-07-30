#pragma once

#if STORM_GC == STORM_GC_SMM

#include "Utils/Lock.h"
#include "Block.h"
#include "InlineSet.h"
#include "Scanner.h"
#include "Arena.h"
#include "Predicates.h"
#include "FinalizerContext.h"

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

			// Get the number of bytes currently allocated in this generation.
			size_t currentAlloc() const { return totalAllocBytes; }

			// Get the number of bytes used by objects etc. in this generation.
			size_t currentUsed() const { return totalAllocBytes - totalFreeBytes; }

			// Get the number of free bytes in this generation.
			size_t currentFree() const { return totalFreeBytes; }

			// Get the number of bytes we're able to allocate before overriding our allocation
			// limit. Returns zero if the limit has already been broken.
			size_t currentGrace() const { return totalSize - min(totalSize, totalAllocBytes) + totalFreeBytes; }

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

			// Scan all blocks that may contain references to anything in the provided GenSet.
			template <class Scanner>
			typename Scanner::Result scan(ArenaTicket &ticket,
										GenSet refsTo,
										typename Scanner::Source &source);

			// Scan all blocks that may contain references to anything in the provided GenSet,
			// possibly raising write barriers since a garbage collection cycle is almost over.
			template <class Scanner>
			typename Scanner::Result scanFinal(ArenaTicket &ticket,
											GenSet refsTo,
											typename Scanner::Source &source);

			// Run all finalizers in this block. Most likely only called before the entire Arena is
			// destroyed, so no need for efficiency.
			void runAllFinalizers(FinalizerContext &context);


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

				// Summary of references in this chunk.
				GenSet summary;

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
				bool compact(ArenaTicket &ticket, const PinnedSet &pinned);

				// Run all finalizers in this chunk.
				void runAllFinalizers(FinalizerContext &context);

				// Memory summary.
				void fillSummary(MemorySummary &summary) const;

				// Verify this chunk.
				void dbg_verify(Arena *arena);

				// Output a summary.
				void dbg_dump();

			private:
				// Called when 'until' has been determined as in use by an object during
				// compaction. Updates the header of the previous block and returns a header to the
				// new current header.
				Block *compactFinishObj(Block *current, fmt::Obj *until);

				// Similar to 'compactFinish', but 'until' is known to be a valid block header. As
				// such, this function does not need to create a new block header, but rather reuses
				// the one already present in 'next'.
				void compactFinishBlock(Block *current, Block *next);

				// Adjust a pointer to an object that is about to be overwritten with 'space' bytes,
				// so that any GcType description objects will not be overwritten. Examines object
				// up to 'end'. Marks any skipped GcType instances as 'dead'.
				static fmt::Obj *skipDeadGcTypes(fmt::Obj *start, fmt::Obj *end, size_t space);

				// Compact an unused block, 'block', keeping pinned objects intact. 'last' is the
				// most recent block header that has been determined to remain. The most recently
				// created block header is returned, as an update to 'last'. Takes care of objects
				// in need of finalization.
				Block *compactPinned(ArenaTicket &ticket, Block *last, Block *block, const PinnedSet &pinned);

				// Shrink a block as much as possible while keeping pinned objects intact. Replaces
				// non-pinned objects with padding. Takes care of objects in need of finalization.
				void shrinkBlock(ArenaTicket &ticket, Block *block, const PinnedSet &pinned);
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
					// Note: -1 so that the first address in a chunk will match the proper chunk, and not the previous one.
					return size_t(a.memory.at) + a.memory.size - 1 < size_t(ptr);
				}
			};

			// Predicate that checks if a particular object is pinned.
			template <class Scanner>
			struct IfPinned : public Scanner {
				struct Source {
					const PinnedSet &pinned;
					typename Scanner::Source &source;

					Source(const PinnedSet &pinned, typename Scanner::Source &source)
						: pinned(pinned), source(source) {}
				};

				PinnedSet pinned;

				IfPinned(Source source) : Scanner(source.source), pinned(source.pinned) {}

				inline ScanOption object(void *start, void *end) {
					if (!pinned.has(start, end))
						return scanNone;

					return Scanner::object(start, end);
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


			/**
			 * Scan a particular GenChunk.
			 *
			 * Note: These are not members of GenChunk since the array of chunks may change during
			 * scanning (e.g. when a scanner allocates memory from the generation currently being
			 * scanned, using ScanState::Move). This means that we need to read all data from the
			 * GenChunk before doing any scanning as the reference could otherwise be
			 * invalidated. This is much easier to get right when the functions are *not* members of
			 * GenChunk, as the dependency becomes more clearly visible then.
			 *
			 * Note: It is interesting to note that block layout might change while scanning due to
			 * new allocations being made in the generation being scanned. Generally, this is fine
			 * as the scanning functions read the size of blocks as late as possible during their
			 * scanning. It can, however, be confusing to see that the size of a block changes
			 * during scanning.
			 */

			template <class Scanner>
			static typename Scanner::Result scan(GenChunk &chunk, typename Scanner::Source &source);

			// Only scan blocks which may refer to objects in the specified generations.
			template <class Scanner>
			static typename Scanner::Result scan(ArenaTicket &ticket,
												GenChunk &chunk,
												GenSet toScan,
												typename Scanner::Source &source);

			// Scan blocks, and possibly raise a write barrier.
			template <class Scanner>
			static typename Scanner::Result scanFinal(ArenaTicket &ticket,
													GenChunk &chunk,
													GenSet toScan,
													typename Scanner::Source &source);

			// Internal version that does not check memory protection for early-outs.
			template <class Scanner>
			static typename Scanner::Result scanImpl(ArenaTicket &ticket,
													GenChunk &chunk,
													GenSet toScan,
													typename Scanner::Source &source);

			// Scan and update the internal summaries simultaneously.
			template <class Scanner>
			static typename Scanner::Result scanUpdate(ArenaTicket &ticket,
													GenChunk &chunk,
													GenSet toScan,
													typename Scanner::Source &source);

			// Scan all pinned objects in this chunk.
			template <class Scanner>
			static typename Scanner::Result scanPinned(GenChunk &chunk,
													const PinnedSet &pinned,
													typename Scanner::Source &source);

			// Scan pinned objects in this chunk, also collecting all blocks marked as
			// containing finalizers in a big list.
			template <class Scanner>
			static typename Scanner::Result scanPinnedFindFinalizers(GenChunk &chunk,
																	const PinnedSet &pinned,
																	typename Scanner::Source &source,
																	Block *&finalizers);

		};


		template <class Scanner>
		typename Scanner::Result Generation::scan(typename Scanner::Source &source) {
			typename Scanner::Result r = typename Scanner::Result();
			for (size_t i = 0; i < chunks.size(); i++) {
				r = scan<Scanner>(chunks[i], source);
				if (r != typename Scanner::Result())
					return r;
			}
			return r;
		}

		template <class Scanner>
		typename Scanner::Result Generation::scan(ArenaTicket &ticket,
												GenSet toScan,
												typename Scanner::Source &source) {
			typename Scanner::Result r = typename Scanner::Result();
			for (size_t i = 0; i < chunks.size(); i++) {
				scan<Scanner>(ticket, chunks[i], toScan, source);
				if (r != typename Scanner::Result())
					return r;
			}
			return r;
		}

		template <class Scanner>
		typename Scanner::Result Generation::scanFinal(ArenaTicket &ticket,
													GenSet toScan,
													typename Scanner::Source &source) {
			typename Scanner::Result r = typename Scanner::Result();
			for (size_t i = 0; i < chunks.size(); i++) {
				scanFinal<Scanner>(ticket, chunks[i], toScan, source);
				if (r != typename Scanner::Result())
					return r;
			}
			return r;
		}


		template <class Scanner>
		typename Scanner::Result Generation::scan(GenChunk &chunk, typename Scanner::Source &source) {
			Block *end = chunk.memory.end();

			typename Scanner::Result r = typename Scanner::Result();
			for (Block *at = (Block *)chunk.memory.at; at != end; at = (Block *)at->mem(at->size)) {
				if (at->hasFlag(Block::fSkipScan)) {
					at->clearFlag(Block::fSkipScan);
					continue;
				}

				r = at->scan<Scanner>(source);
				if (r != typename Scanner::Result())
					return r;
			}
			return r;
		}

		template <class Scanner>
		typename Scanner::Result Generation::scan(ArenaTicket &ticket,
												GenChunk &chunk,
												GenSet toScan,
												typename Scanner::Source &source) {
			// No changes? Then we can use the summary!
			if (!ticket.anyWrites(chunk.memory)) {
				// Not interesting? If so, we can just bail out.
				if (!chunk.summary.has(toScan)) {
					return typename Scanner::Result();
				}
			}

			return scanImpl<Scanner>(ticket, chunk, toScan, source);
		}

		template <class Scanner>
		typename Scanner::Result Generation::scanFinal(ArenaTicket &ticket,
													GenChunk &chunk,
													GenSet toScan,
													typename Scanner::Source &source) {

			// Was this block changed since we last prepared our summary?
			if (!ticket.anyWrites(chunk.memory)) {
				// No changes, we can ask the summary!
				if (chunk.summary.has(toScan)) {
					return scanImpl<Scanner>(ticket, chunk, toScan, source);
				} else {
					return typename Scanner::Result();
				}
			}

			// TODO: We need to determine if we should update the summary or not.
			bool update = true;
			if (update)
				return scanUpdate<Scanner>(ticket, chunk, toScan, source);
			else
				return scanImpl<Scanner>(ticket, chunk, toScan, source);
		}

		template <class Scanner>
		typename Scanner::Result Generation::scanImpl(ArenaTicket &ticket,
													GenChunk &chunk,
													GenSet toScan,
													typename Scanner::Source &source) {

			typename Scanner::Result r = typename Scanner::Result();
			Block *end = (Block *)chunk.memory.end();
			for (Block *at = (Block *)chunk.memory.at; at != end; at = (Block *)at->mem(at->size)) {
				if (at->hasFlag(Block::fSkipScan)) {
					at->clearFlag(Block::fSkipScan);
					continue;
				}

				if (!at->mayReferTo(ticket, toScan))
					continue;

				r = at->scan<Scanner>(source);
				if (r != typename Scanner::Result())
					return r;
			}

			return r;
		}

		template <class Scanner>
		typename Scanner::Result Generation::scanUpdate(ArenaTicket &ticket,
													GenChunk &chunk,
													GenSet toScan,
													typename Scanner::Source &source) {

			GenSet summary;

			typename Scanner::Result r = typename Scanner::Result();
			Block *end = (Block *)chunk.memory.end();
			for (Block *at = (Block *)chunk.memory.at; at != end; at = (Block *)at->mem(at->size)) {
				// We need to scan these... (Some memory might be protected, so needlessly writing might be a bad idea).
				if (at->hasFlag(Block::fSkipScan))
					at->clearFlag(Block::fSkipScan);

				if (at->mayReferTo(ticket, toScan)) {
					r = at->scanUpdate<Scanner>(ticket, source);
					if (r != typename Scanner::Result())
						return r;
				}

				summary.add(at->genSummary());
			}

			// Update memory protection and remember the references.
			chunk.summary = summary;
			ticket.watchWrites(chunk.memory);

			return r;
		}

		template <class Scanner>
		typename Scanner::Result Generation::scanPinned(GenChunk &chunk,
														const PinnedSet &pinned,
														typename Scanner::Source &source) {

			typename Scanner::Result r = typename Scanner::Result();
			if (pinned.empty())
				return r;

			typedef IfPinned<Scanner> Pinned;
			typename Pinned::Source s(pinned, source);

			// Walk all blocks and scan the relevant ones.
			Block *end = (Block *)chunk.memory.end();
			for (Block *at = (Block *)chunk.memory.at; at != (Block *)end; at = (Block *)at->mem(at->size)) {
				if (pinned.has(at->mem(0), at->mem(at->committed()))) {
					r = at->scan<Pinned>(s);
					if (r != typename Scanner::Result())
						return r;
				}
			}

			return r;
		}

		template <class Scanner>
		typename Scanner::Result Generation::scanPinnedFindFinalizers(GenChunk &chunk,
																	const PinnedSet &pinned,
																	typename Scanner::Source &source,
																	Block *&finalizers) {

			typename Scanner::Result r = typename Scanner::Result();

			typedef IfPinned<Scanner> Pinned;
			typename Pinned::Source s(pinned, source);

			// Walk all blocks and scan the relevant ones.
			Block *end = (Block *)chunk.memory.end();
			for (Block *at = (Block *)chunk.memory.at; at != (Block *)end; at = (Block *)at->mem(at->size)) {
				if (pinned.has(at->mem(0), at->mem(at->committed()))) {
					r = at->scan<Pinned>(s);
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
