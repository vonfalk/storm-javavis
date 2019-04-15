#include "stdafx.h"
#include "Generation.h"

#if STORM_GC == STORM_GC_SMM

#include "Arena.h"
#include "Scanner.h"
#include "ObjWalk.h"

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

			return pos->allocBlock(minSize, maxSize, minFragment());
		}

		void Generation::collect(ArenaEntry &entry) {}

		void Generation::dbg_verify() {
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
