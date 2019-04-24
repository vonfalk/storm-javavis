#include "stdafx.h"
#include "Generation.h"

#if STORM_GC == STORM_GC_SMM

#include "Arena.h"
#include "Scanner.h"
#include "ScanState.h"

namespace storm {
	namespace smm {

		// TODO: What is a reasonable block size here?
		Generation::Generation(Arena &owner, size_t size, byte identifier)
			: totalSize(size), blockSize(size / 32),
			  next(null), owner(owner), identifier(identifier), sharedBlock(null) {}

		Generation::~Generation() {
			// Free all blocks we are in charge of.
			for (size_t i = 0; i < chunks.size(); i++) {
				owner.freeChunk(chunks[i].memory);
			}
		}

		Block *Generation::alloc(size_t minSize) {
			util::Lock::L z(lock);
			return allocBlock(minSize, blockSize);
		}

		void Generation::done(Block *block) {
			util::Lock::L z(lock);
			block->reserved(block->committed());

			ChunkList::iterator pos = std::lower_bound(chunks.begin(), chunks.end(), block, PtrCompare());
			if (pos != chunks.end()) {
				pos->releaseBlock(block);
			} else {
				assert(false, L"Calling 'done' for a block not belonging to this generation!");
			}

			// TODO: Look at the amount of memory we're using, and consider triggering a garbage
			// collection, at least for this generation.
		}

		bool Generation::isPinned(void *obj, void *end) {
			ChunkList::iterator pos = std::lower_bound(chunks.begin(), chunks.end(), obj, PtrCompare());
			if (pos != chunks.end()) {
				return pinnedSets[pos - chunks.begin()].has(obj, end);
			} else {
				// Should not happen...
				return false;
			}
		}

		Block *Generation::fillBlock(size_t size) {
			if (sharedBlock && sharedBlock->remaining() < size) {
				done(sharedBlock);
				sharedBlock = null;
			}

			if (!sharedBlock)
				sharedBlock = allocBlock(size, max(size + (size >> 1), blockSize));

			return sharedBlock;
		}

		Block *Generation::allocBlock(size_t minSize, size_t maxSize) {
			if (minSize > maxSize)
				return null;

			// Find a chunk where the allocation may fit.
			for (size_t i = 0; i < chunks.size(); i++) {
				if (Block *r = chunks[i].allocBlock(minSize, maxSize, minFragment()))
					return r;

				if (++lastChunk == chunks.size())
					lastChunk = 0;
			}

			// We need to allocate more memory. TODO: How much?
			Chunk c = owner.allocChunk(blockSize * 32, identifier);

			// Out of memory?
			if (c.empty())
				return null;

			// Find a place to insert the new chunk!
			ChunkList::iterator pos = std::lower_bound(chunks.begin(), chunks.end(), c, ChunkCompare());
			// TODO: We might want to merge blocks, but that requires us to be able to split and
			// deallocate unused blocks as well...
			pos = chunks.insert(pos, GenChunk(c));

			while (pinnedSets.size() < chunks.size())
				pinnedSets.push_back(PinnedSet(0, 1));

			return pos->allocBlock(minSize, maxSize, minFragment());
		}

		struct IfInGen {
			Arena &arena;
			byte id;

			IfInGen(Generation *gen) : arena(gen->owner), id(gen->identifier) {}

			inline bool operator ()(void *ptr) const {
				return arena.memGeneration(ptr) == id;
			}
		};

		void Generation::collect(ArenaEntry &entry) {
			if (!next || next == this) {
				TODO(L"Set up generations so that all generations have a 'next' generation!");
				return;
			}

			// No need for GC if we're empty.
			if (chunks.empty())
				return;

			// Note: This assumes a generation where objects may move. Non-moving objects need to be
			// treated differently (especially since we don't expect there to be very many of them).

			// Scan all non-exact roots (the stacks of all threads), and keep track of non-moving
			// objects.

			for (size_t i = 0; i < chunks.size(); i++)
				pinnedSets[i] = chunks[i].memory.addrSet<PinnedSet>();

			// TODO: We might want to do an 'early out' inside the scanning by using
			// VMAlloc::identifier before attempting to access the sets. We need to measure the benefits of this!
			entry.scanStackRoots<ScanSummaries<PinnedSet>>(pinnedSets);

			// Keep track of surviving objects inside a ScanState object, which allocates memory
			// from the next generation.
			ScanState state(this, next);

			// For all blocks containing at least one pinned object, traverse it entirely to find
			// and scan the pinned objects. Any non-pinned objects are copied to the new block.
			for (size_t i = 0; i < chunks.size(); i++)
				chunks[i].scanPinned(pinnedSets[i], state);

			// Traverse all other generations that could contain references to this generation and
			// copy any referred objects to the new block.
			entry.scanGenerations<ScanState::Move>(state, this);

			// Traverse the newly copied objects in the new block and copy any new references until
			// no more objects are found.
			state.scanNew();

			// Update any references to objects we just moved. Note: For this to work, we must make
			// sure that the newly created blocks in the new generation are scanned by this statement!
			entry.scanGenerations<UpdateFwd<IfInGen>>(IfInGen(this), this);
			scan<UpdateFwd<IfInGen>>(IfInGen(this));

			dbg_dump();

			// Finally, release and/or compact any remaining blocks in this generation.
			for (size_t i = 0; i < chunks.size(); i++) {
				GenChunk &chunk = chunks[i];
				PinnedSet &pinned = pinnedSets[i];

				// TODO: We need to consider objects with finalizers as well!
				if (chunk.compact(pinned)) {
					owner.freeChunk(chunk.memory);
					chunks.erase(chunks.begin() + i);
					i--;
				}
			}

			dbg_dump();
		}

		void Generation::dbg_verify() {
			assert(chunks.size() == pinnedSets.size());

			for (size_t i = 0; i < chunks.size(); i++) {
				if (i > 0) {
					assert((byte *)chunks[i - 1].memory.at < (byte *)chunks[i].memory.at, L"Unordered chunks!");
				}

				chunks[i].dbg_verify();
			}
		}

		void Generation::dbg_dump() {
			PLN(L"Generation with id " << identifier << L", " << chunks.size() << L" chunks:");
			for (size_t i = 0; i < chunks.size(); i++) {
				PNN(i << L": ");
				chunks[i].dbg_dump();
			}
		}


		/**
		 * A chunk.
		 */

		Generation::GenChunk::GenChunk(Chunk chunk) : memory(chunk) {
			lastAlloc = new (memory.at) Block(memory.size - sizeof(Block));
			freeBytes = memory.size - sizeof(Block);

			// TODO: We might want to use one of the free chunks to contain a data structure to
			// accelerate allocations! That memory is committed anyway, so we might as well use it.
		}

		static inline Block *next(Block *b) {
			return (Block *)b->mem(b->size);
		}

		static inline Block *nextWrap(Block *b, const Chunk &limit) {
			Block *n = next(b);
			if ((size_t)n - (size_t)limit.at >= limit.size)
				n = (Block *)limit.at;
			return n;
		}

		Block *Generation::GenChunk::allocBlock(size_t minSize, size_t maxSize, size_t minFragment) {
			// We will definitely fail to find a suitable allocation spot...
			if (freeBytes < minSize)
				return null;

			// Visit all elements in turn, starting at 'lastAlloc' and try to find something good!
			bool first = true;
			for (Block *at = lastAlloc; first || at != lastAlloc; at = nextWrap(at, memory), first = false) {
				// If it is used by an allocator, skip it.
				if (at->hasFlag(Block::fUsed))
					continue;

				// Not enough space?
				size_t remaining = at->remaining();
				if (remaining < minSize)
					continue;

				// Split the block?
				if (remaining > maxSize && remaining > minSize + minFragment + sizeof(Block)) {
					// Split at either the maximum size, or at the size that yeilds 'minFragment'
					// bytes in the other block.
					size_t splitAfter = min(maxSize, remaining - minFragment - sizeof(Block));
					size_t used = at->committed();

					// Create the new block.
					new (at->mem(used + splitAfter)) Block(at->size - used - splitAfter - sizeof(Block));
					freeBytes -= sizeof(Block);

					// Re-create the old block to change its size.
					new (at) Block(used + splitAfter);
					at->committed(used);
					at->reserved(used);
				}

				// This is the one we want!
				at->setFlag(Block::fUsed);
				freeBytes -= at->remaining();
				lastAlloc = at;
				return at;
			}

			// No block found.
			return null;
		}

		void Generation::GenChunk::releaseBlock(Block *block) {
			block->clearFlag(Block::fUsed);
			freeBytes += block->remaining();
		}

		bool Generation::GenChunk::compact(const PinnedSet &pinned) {
			// The block that is currently being expanded.
			Block *current = (Block *)memory.at;

			// Reset internal data structures.
			lastAlloc = current;
			freeBytes = 0;

			// Traverse the blocks inside this chunk to find things we need to preserve. We need to
			// keep chunks marked as used and pinned objects. The rest is known to be free. We do,
			// however, need to consider finalizers.
			Block *at = (Block *)memory.at;
			while (at != (Block *)memory.end()) {
				// We need to read this first. Otherwise, we might destroy the old value when we're compacting!
				Block *next = (Block *)at->mem(at->size);

				if (at->hasFlag(Block::fUsed)) {
					// Used block, we need to keep this pointer valid and the size
					// constant. However, we are free to decrease 'committed' if we are able to!

					compactFinishBlock(current, at);
					shrinkBlock(at, pinned);

					// Skip ahead, don't touch the size of this block anymore.
					current = (Block *)at->mem(at->size);
				} else if (pinned.has(at->mem(0), at->mem(at->committed()))) {
					// Unused block with pinned objects. We need to preserve those...
					current = compactPinned(current, at, pinned);
				} else {
					// Unused block. We are free to remove this block entirely without looking at it anymore.
					current->committed(0);
					current->reserved(0);
				}

				at = next;
			}

			// Finish the last block if necessary.
			compactFinishBlock(current, (Block *)memory.end());

			// If the compaction resulted in a single empty block that is not marked as used, we're empty.
			Block *first = (Block *)memory.at;
			return first->mem(first->size) == memory.end()    // one block
				&& first->committed() == 0                    // empty
				&& !first->hasFlag(Block::fUsed);             // not in use
		}

		Block *Generation::GenChunk::compactFinishObj(Block *current, void *until) {
			// Usable size in 'current'. If we are unable to fit a second block header there, we
			// need to merge them into one large block instead!
			size_t size = (byte *)until - (byte *)current - sizeof(Block);
			size_t used = current->committed();

			if (size <= sizeof(Block) + used) {
				// One large object. Just add a padding object and continue!
				fmt::objMakePad((fmt::Obj *)current->mem(used), size - used);
				current->committed(size);
				current->reserved(size);
				return current;
			}

			// We are better off creating a new object, so that we can use the gap!
			size -= sizeof(Block);
			current = new (current) Block(size);
			current->committed(used);
			current->reserved(used);

			freeBytes += current->remaining();

			// Create a new block with a temporary size of zero. It will be recreated later with the
			// proper size, and we just need a header! Note: Block initializes 'committed' and
			// 'reserved' to zero.
			return new (current->mem(size)) Block(0);
		}

		void Generation::GenChunk::compactFinishBlock(Block *current, Block *next) {
			// If they are the same, just ignore the request.
			if (current == next)
				return;

			// Note: If we get past the previous check, we know that 'current' does not have the
			// 'used' flag set due to how 'compact' works. Thus, we can ignore the flags in the block.
			size_t size = (byte *)next - (byte *)current - sizeof(Block);
			size_t used = current->committed();

			current = new (current) Block(size);
			current->committed(used);
			current->reserved(used);

			freeBytes += current->remaining();
		}

		Block *Generation::GenChunk::compactPinned(Block *current, Block *scan, const PinnedSet &pinned) {
			fmt::Obj *at = (fmt::Obj *)scan->mem(0);
			fmt::Obj *to = (fmt::Obj *)scan->mem(scan->committed());

			// Find rising and falling 'edges' in the usage, and structure the blocks accordingly.
			bool lastInUse = false;

			while (at != to) {
				fmt::Obj *next = fmt::objSkip(at);
				bool inUse = pinned.has(fmt::toClient(at), next);

				if (inUse & !lastInUse) {
					// First object in use. We might want to start a new block.
					current = compactFinishObj(current, at);
				} else if (!inUse & lastInUse) {
					// Last object in a hunk, mark the size accordingly.
					size_t sz = (byte *)next - (byte *)current->mem(0);
					current->committed(sz);
					current->reserved(sz);
				}

				lastInUse = inUse;
				at = next;
			}

			// If the last few objects were in use, mark the size accordingly.
			if (lastInUse) {
				size_t sz = (byte *)to - (byte *)current->mem(0);
				current->committed(sz);
				current->reserved(sz);
			}

			return current;
		}

		void Generation::GenChunk::shrinkBlock(Block *block, const PinnedSet &pinned) {
			fmt::Obj *at = (fmt::Obj *)block->mem(0);
			fmt::Obj *to = (fmt::Obj *)block->mem(block->committed());

			// First free location as far as we know.
			fmt::Obj *freeFrom = at;

			while (at != to) {
				fmt::Obj *next = fmt::objSkip(at);

				if (pinned.has(fmt::toClient(at), next)) {
					// Replace previous objects with padding?
					size_t padSize = (byte *)at - (byte *)freeFrom;
					if (padSize > fmt::headerSize) {
						fmt::objMakePad(freeFrom, padSize);
					}

					// Remember where the free space starts.
					freeFrom = next;
				}

				at = next;
			}

			size_t used = (byte *)at - (byte *)freeFrom;
			block->committed(used);
			block->reserved(used);
		}

		struct IfPinned {
			IfPinned(const PinnedSet &pinned) : pinned(pinned) {}

			PinnedSet pinned;

			bool operator() (void *from, void *to) const {
				return pinned.has(from, to);
			}
		};

		void Generation::GenChunk::scanPinned(const PinnedSet &pinned, ScanState &scan) {
			if (pinned.empty())
				return;

			// Walk all blocks and scan the relevant ones.
			for (Block *at = (Block *)memory.at; at != (Block *)memory.end(); at = (Block *)at->mem(at->size)) {
				if (pinned.has(at->mem(0), at->mem(at->committed())))
					at->scanIf<IfPinned, ScanState::Move>(pinned, scan);
			}
		}

		void Generation::GenChunk::dbg_verify() {
			size_t free = 0;
			Block *at = (Block *)memory.at;
			while ((byte *)at < (byte *)memory.end()) {
				assert(memory.has(at), L"Invalid block size detected. Leads to outside the specified chunk!");

				if (!at->hasFlag(Block::fUsed))
					free += at->remaining();

				at->dbg_verify();
				at = (Block *)at->mem(at->size);
			}

			assert(at == memory.end(), L"A chunk is overfilled!");
			assert(free == freeBytes, L"The number of free bytes is not correct. "
				L"Stored: " + ::toS(freeBytes) + L", actual: " + ::toS(free));
		}

		void Generation::GenChunk::dbg_dump() {
			PLN(L"Chunk at " << memory << L", " << freeBytes << L" bytes free");

			for (Block *at = (Block *)memory.at; at != (Block *)memory.end(); at = (Block *)at->mem(at->size)) {
				if (at == lastAlloc)
					PNN(L"-> ");
				else
					PNN(L"   ");

				at->dbg_dump();
			}
		}

	}
}

#endif
