#include "stdafx.h"
#include "BlockAlloc.h"

#if STORM_GC == STORM_GC_SMM

#include "Gc/Gc.h"

namespace storm {
	namespace smm {

		/**
		 * The header of a chunk for convenient access.
		 */
		struct ChunkHeader {
			// Number of free pages.
			size_t freePages;

			// Location of the next potential allocation. We're currently using a "round-robin"
			// allocation strategy.
			size_t nextAlloc;

			// The actual data. One bit per page.
			size_t data[1];
		};

		static const size_t wordBits = sizeof(size_t) * CHAR_BIT;

		// Get the index of the highest set bit in a number, assuming at least one.
		static size_t highestSetBit(size_t s) {
			size_t count = 0;
			while (s) {
				s >>= 1;
				count++;
			}
			return count - 1;
		}

		// Find a range of free bits in a bitmask. Returns 'length' if none present.
		static size_t findFree(ChunkHeader *header, size_t length, size_t find) {
			// Start of a free section of pages.
			size_t start = header->nextAlloc;

			// Page index currently being examined.
			size_t at = start;
			while (true) {
				size_t index = at / wordBits;
				size_t bit = at % wordBits;

				// # of free pages needed in this word.
				size_t needed = min(find - (at - start), wordBits - bit);
				size_t mask = std::numeric_limits<size_t>::max() >> (wordBits - needed);
				mask <<= bit;

				size_t obstacles = header->data[index] & mask;
				if (obstacles) {
					// Not enough free space. Set 'start' to the position of the highest bit found,
					// and continue searching from there.
					size_t next = index * wordBits + highestSetBit(obstacles) + 1;

					// If we attempt to move 'start' to or beyond 'header->nextAlloc' from a place
					// before 'header->nextAlloc', we know that there is no room for our allocation,
					// and we're done.
					if (next >= header->nextAlloc && start < header->nextAlloc)
						return null;
					continue;
				}

				if (at - start + needed >= find) {
					// Done!
					return start;
				}

				// Keep going, we need to check that there is room for everything!
				at += needed;

				// Do we need to wrap around?
				if (at >= length)
					at = start = 0;
			}
		}

		// Mark a series of pages.
		static void mark(ChunkHeader *header, size_t start, size_t length) {
			while (length > 0) {
				size_t index = start / wordBits;
				size_t bit = start % wordBits;

				size_t needed = min(length, wordBits - bit);
				size_t mask = std::numeric_limits<size_t>::max() >> (wordBits - needed);
				mask <<= bit;

				header->data[index] |= mask;

				start += needed;
				length -= needed;
			}
		}

		// Clear a series of pages.
		static void clear(ChunkHeader *header, size_t start, size_t length) {
			while (length > 0) {
				size_t index = start / wordBits;
				size_t bit = start % wordBits;

				size_t needed = min(length, wordBits - bit);
				size_t mask = std::numeric_limits<size_t>::max() >> (wordBits - needed);
				mask <<= bit;

				header->data[index] &= ~mask;

				start += needed;
				length -= needed;
			}
		}


		BlockAlloc::BlockAlloc(VM *vm, size_t initSize) : vm(vm), pageSize(vm->pageSize) {
			// Try to allocate the initial block!
			initSize = roundUp(initSize, vm->allocGranularity);
			void *mem = vm->reserve(null, initSize);
			while (!mem) {
				initSize /= 2;
				if (initSize < vm->allocGranularity)
					throw GcError(L"Unable to allocate memory for the initial arena!");
				mem = vm->reserve(null, initSize);
			}
			addChunk(mem, initSize);
		}

		BlockAlloc::~BlockAlloc() {
			for (size_t i = 0; i < chunks.size(); i++)
				vm->free(chunks[i].at, chunks[i].pages * pageSize);

			delete vm;
		}

		size_t BlockAlloc::headerSize(size_t size) {
			size_t pages = (size + pageSize - 1) / pageSize;
			size_t bytes = (pages + CHAR_BIT - 1) / CHAR_BIT;
			size_t total = sizeof(ChunkHeader) - sizeof(size_t) + bytes;
			return roundUp(total, pageSize);
		}

		void BlockAlloc::addChunk(void *mem, size_t size) {
			Chunk chunk(mem, size / pageSize, headerSize(size));
			ChunkHeader *header = chunk.header();

			// Commit the memory for the header.
			vm->commit(mem, chunk.headerSize);

			// We don't need to initialize it to zero, the OS will do this for us. We do need some
			// initialization, though.
			header->freePages = (size - chunk.headerSize) / pageSize;
			header->nextAlloc = 0;

			// Add it to our list of chunks!
			chunks.push_back(chunk);
		}

		BlockAlloc::Chunk *BlockAlloc::findChunk(void *ptr) {
			// TODO: We could sort the chunks to implement this lookup faster!
			size_t p = size_t(ptr);
			for (size_t i = 0; i < chunks.size(); i++) {
				Chunk *c = &chunks[i];
				if (size_t(c->at) <= p && size_t(c->at) + c->pages * pageSize > p)
					return c;
			}

			return null;
		}

		Block *BlockAlloc::alloc(size_t size) {
			// Add the Block's header.
			size += sizeof(Block);

			// Round up to nearest multiple and compute # of pages.
			size_t pages = (size + pageSize - 1) / pageSize;

			// A simple 'first-fit' algorithm should be enough. We assume that there are few (< 5) chunks in the array.
			for (size_t i = 0; i < chunks.size(); i++) {
				Chunk &c = chunks[i];
				if (c.header()->freePages < pages)
					continue;

				if (Block *b = alloc(c, pages))
					return b;
			}

			assert(false, L"TODO: Try to allocate a new chunk!");
			return null;
		}

		Block *BlockAlloc::alloc(Chunk &c, size_t pages) {
			// Try to find a sequence of length 'pages' in the bitmap, starting at 'nextAlloc'.
			size_t found = findFree(c.header(), c.pages, pages);
			if (found >= c.pages)
				return null;

			// Mark them as used!
			mark(c.header(), found, pages);
			c.header()->nextAlloc = found + pages;

			// Commit the memory, and return.
			void *mem = c.memory(found * pageSize);
			vm->commit(mem, pages * pageSize);
			return new (mem) Block(pages * pageSize - sizeof(Block));
		}

		void BlockAlloc::free(Block *block) {
			Chunk *c = findChunk(block);
			if (c) {
				size_t size = sizeof(Block) + block->size;
				vm->decommit(block, size);

				size_t page = (size_t(block) - size_t(c->at)) / pageSize;
				clear(c->header(), page, size / pageSize);
			} else {
				throw GcError(L"Invalid pointer passed to 'free'");
			}
		}

	}
}

#endif
