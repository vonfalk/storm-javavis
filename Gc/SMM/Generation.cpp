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

		void Generation::collect(ArenaEntry &entry) {
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
			// entry.scanGenerations<ScanState::Move>(state, this);

			// Traverse the newly copied objects in the new block and copy any new references until
			// no more objects are found.

			// Update any references to objects we just moved.

			// Finally, release and/or compact any remaining blocks in this generation.
		}

		void Generation::dbg_verify() {
			assert(chunks.size() == pinnedSets.size());

			for (size_t i = 0; i < chunks.size(); i++) {
				chunks[i].dbg_verify();
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
					size_t used = atomicRead(at->committed);

					// Create the new block.
					new (at->mem(used + splitAfter)) Block(at->size - used - splitAfter - sizeof(Block));
					freeBytes -= sizeof(Block);

					// Re-create the old block to change its size.
					new (at) Block(used + splitAfter);
					at->committed = at->reserved = used;
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
				if (pinned.has(at->mem(0), at->mem(at->committed)))
					at->scanIf<IfPinned, ScanState::Move>(pinned, scan);
			}
		}

		void Generation::GenChunk::dbg_verify() {
			Block *at = (Block *)memory.at;
			while ((byte *)at < (byte *)memory.end()) {
				assert(memory.has(at), L"Invalid block size detected. Leads to outside the specified chunk!");

				at->dbg_verify();
				at = (Block *)at->mem(at->size);
			}

			assert(at == memory.end(), L"A chunk is overfilled!");
		}

	}
}

#endif
