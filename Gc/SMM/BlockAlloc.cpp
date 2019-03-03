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
			// Size of the header (in bytes), rounded to the nearest page boundary.
			size_t size;

			// Number of free pages.
			size_t freePages;

			// Location of the next potential allocation. We're currently using a "round-robin"
			// allocation strategy.
			size_t nextAlloc;

			// The bitmap containing used pages. One bit per page (must be the last member as it is inlined).
			size_t used[1];
		};

		static const size_t wordBits = sizeof(size_t) * CHAR_BIT;

		// Compute the offset of the block-start table located after 'used'.
		static size_t blockStartOffset(size_t pages) {
			size_t usedWords = (pages + wordBits - 1) / wordBits;
			size_t total = sizeof(ChunkHeader) - sizeof(size_t);
			total += wordBits * sizeof(size_t);
			return total;
		}

		// Compute the size of a ChunkHeader.
		static size_t headerSize(size_t pages) {
			// Words in the used-table.
			size_t usedWords = (pages + wordBits - 1) / wordBits;

			// The header itself.
			size_t total = sizeof(ChunkHeader) - sizeof(size_t);
			// Size of the 'used' table.
			total += wordBits * sizeof(size_t);
			// Size of the Block-start table (one additional byte for each element in 'used').
			total += wordBits;

			return total;
		}

		// Get a pointer to the first element in the block-start table.
		static byte *blockStart(ChunkHeader *header, size_t pages) {
			return (byte *)header + blockStartOffset(pages);
		}

		// Get the index of the highest set bit in a number, assuming at least one.
		static size_t highestSetBit(size_t s) {
			size_t count = 0;
			while (s) {
				s >>= 1;
				count++;
			}
			return count - 1;
		}

		// Get the index of the lowest set bit in a number, assuming at least one. Compared to
		// 'trailingZeros', this implementation does not require the input to be a power of two.
		static size_t lowestSetBit(size_t s) {
			size_t count = 0;
			while ((s & 0x1) == 0x0) {
				s >>= 1;
				count++;
			}
			return count;
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

				size_t obstacles = header->used[index] & mask;
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

		// Mark a series of pages. Also makes sure to update the block-start table accordingly.
		static void mark(ChunkHeader *header, size_t numPages, size_t start, size_t length) {
			size_t at = start;
			byte *bStart = blockStart(header, numPages);

			while (length > 0) {
				size_t index = at / wordBits;
				size_t bit = at % wordBits;

				// Update the used bitmask.
				size_t needed = min(length, wordBits - bit);
				size_t mask = std::numeric_limits<size_t>::max() >> (wordBits - needed);
				mask <<= bit;

				header->used[index] |= mask;

				// Update the block-start table.
				if (at == start) {
					// First block, update the entry so that it points to us if we're first.
					bStart[index] = min(bStart[index], byte(bit));
				} else if (needed == length) {
					// Last block. No need to update that.
				} else {
					// Middle block. Set it to 0xFF (should already be the case).
					bStart[index] = 0xFF;
				}

				// Next element.
				at += needed;
				length -= needed;
			}
		}

		// Clear a series of pages. Also makes sure to update the block-start table accordingly.
		static void clear(ChunkHeader *header, size_t numPages, size_t start, size_t length) {
			size_t at = start;
			byte *bStart = blockStart(header, numPages);

			while (length > 0) {
				size_t index = at / wordBits;
				size_t bit = at % wordBits;

				// Update the used bitmask.
				size_t needed = min(length, wordBits - bit);
				size_t mask = std::numeric_limits<size_t>::max() >> (wordBits - needed);
				mask <<= bit;

				header->used[index] &= ~mask;

				// Update the block-start table.
				if (at == start) {
					// First block. Increase the entry if it referred to us.
					if (bStart[index] == bit) {
						// See if this element contains the start of a block or not.
						size_t used = header->used[index] & (std::numeric_limits<size_t>::max() << bit);
						if (used == 0)
							bStart[index] = 0xFF;
						else
							bStart[index] = lowestSetBit(used);
					}
				} else if (needed == length) {
					// Last block. No need to update that.
				} else {
					// Middle block. Set it to 0xFF (should already be the case).
					bStart[index] = 0xFF;
				}

				// Next element.
				at += needed;
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

		void BlockAlloc::updateMinMax() {
			if (chunks.empty()) {
				minAddr = 0;
				maxAddr = 1;
			}

			minAddr = size_t(chunks[0].at);
			maxAddr = minAddr + chunks[0].pages * pageSize;

			for (size_t i = 1; i < chunks.size(); i++) {
				size_t base = size_t(chunks[i].at);
				minAddr = min(minAddr, base);
				maxAddr = max(maxAddr, base + chunks[i].pages * pageSize);
			}
		}

		void BlockAlloc::addChunk(void *mem, size_t size) {
			size_t pages = (size + pageSize - 1) / pageSize;
			size_t headerSz = roundUp(headerSize(pages), pageSize);
			Chunk chunk(mem, pages);
			ChunkHeader *header = chunk.header();

			// Commit the memory for the header.
			vm->commit(mem, headerSz);

			// We don't need to initialize it to zero, the OS will do this for us. We do need some
			// initialization, though.
			header->size = headerSz;
			header->freePages = (size - headerSz) / pageSize;
			header->nextAlloc = 0;

			// Initialize the block-start table to 0xFF.
			memset(blockStart(header, pages), 0xFF, pages / wordBits);

			// Add it to our list of chunks!
			chunks.push_back(chunk);
			updateMinMax();
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

		void *BlockAlloc::pageToAddr(ChunkHeader *header, size_t offset) const {
			return (byte *)header + offset*pageSize + header->size;
		}

		size_t BlockAlloc::addrToPage(ChunkHeader *header, void *addr) const {
			size_t offset = (byte *)addr - (byte *)header;
			return (offset - header->size) / pageSize;
		}

		size_t BlockAlloc::addrToPageSafe(ChunkHeader *header, void *addr) const {
			size_t offset = (byte *)addr - (byte *)header;
			return (max(offset, header->size) - header->size) / pageSize;
		}

		Block *BlockAlloc::alloc(Chunk &c, size_t pages) {
			// Try to find a sequence of length 'pages' in the bitmap, starting at 'nextAlloc'.
			size_t found = findFree(c.header(), c.pages, pages);
			if (found >= c.pages)
				return null;

			// Mark them as used!
			mark(c.header(), c.pages, found, pages);
			c.header()->nextAlloc = found + pages;
			c.header()->freePages -= pages;

			// Commit the memory, and return.
			void *mem = pageToAddr(c.header(), found);
			vm->commit(mem, pages * pageSize);
			return new (mem) Block(this, pages * pageSize - sizeof(Block));
		}

		void BlockAlloc::free(Block *block) {
			Chunk *c = findChunk(block);
			if (c) {
				size_t size = sizeof(Block) + block->size;
				vm->decommit(block, size);

				size_t page = addrToPage(c->header(), block);
				size_t pageCount = size / pageSize;
				clear(c->header(), c->pages, page, pageCount);
				c->header()->freePages += pageCount;
			} else {
				throw GcError(L"Invalid pointer passed to 'free'");
			}
		}

		void BlockAlloc::checkWrites(void **buffer) {
			vm->notifyWrites(this, buffer);
		}

		void BlockAlloc::watchWrites(Block *block) {
			vm->watchWrites(this, block, sizeof(Block) + block->size);
		}

		Block *BlockAlloc::findBlock(void *ptr) {
			Chunk *c = findChunk(ptr);
			if (!c)
				return null;

			byte *bStart = blockStart(c->header(), c->pages);

			size_t page = addrToPage(c->header(), ptr);
			size_t index = page / wordBits;

			// Does the index directly here contain the block we're looking for?
			if (bStart[index] > page % wordBits) {
				// No, it is either 0xFF or the first block in this index is after the one we're
				// looking for. Walk backwards to find the start of the block!
				do {
					// Nothing to find?
					if (index == 0)
						return null;
					index--;
				} while (bStart[index] != 0xFF);
			}

			// Now, we have an element in bStart that indicates the start of the first block inside
			// this index. Now, we just need to iterate through the blocks (in case there are
			// multiple) to find the one we're looking for.
			size_t offset = bStart[index];
			while (offset < wordBits) {
				if ((c->header()->used[index] >> offset) & 0x1) {
					Block *block = (Block *)pageToAddr(c->header(), index*wordBits + offset);
					size_t size = sizeof(Block) + block->size;
					void *start = block;
					void *end = (byte *)start + size;

					if (size_t(start) <= size_t(ptr) && size_t(end) > size_t(ptr)) {
						return block;
					} else if (size_t(end) > size_t(ptr)) {
						// This means that the start of the next block will be too large, so we might as
						// well quit early.
						return null;
					}

					// Go to the next one. Note: size is always a multiple of 'pageSize'.
					offset += size / pageSize;
				} else {
					// The page is not in use. Go to the next one!
					offset++;
				}
			}

			// Nothing found.
			return null;
		}

		struct AddrCmp {
			bool operator ()(void *l, void *r) const {
				return size_t(l) < size_t(r);
			}
		};

		void BlockAlloc::markBlocks(void **addr, size_t count) {
			if (count <= 0)
				return;

			// Partition the addresses to each of the chunks, so we don't have to deal with that later.
			for (size_t i = 0; i < chunks.size(); i++) {
				Chunk &c = chunks[i];

				void **first = std::lower_bound(addr, addr + count, c.at, AddrCmp());
				void **last = std::lower_bound(addr, addr + count, (char *)c.at + c.pages*pageSize, AddrCmp());

				if (first != last)
					markBlocks(c, first, last);
			}
		}

		void BlockAlloc::markBlocks(Chunk &c, void **begin, void **end) {
			byte *bStart = blockStart(c.header(), c.pages);

			// We have a loop here since we might want to re-start if there are large enogugh gaps
			// in the address ranges supplied. Otherwise, we may end up traversing large parts of
			// the memory even though there are only a few blocks we actually need to mark!
			while (begin < end) {
				size_t page = addrToPageSafe(c.header(), *begin);
				size_t index = page / wordBits;

				size_t lastPage = addrToPageSafe(c.header(), *(end - 1));
				size_t lastIndex = lastPage / wordBits;

				// Does the index directly here contain the block we're looking for?
				if (bStart[index] > page % wordBits) {
					// No, it is either 0xFF or the first block in this index is after the one we're
					// looking for. Walk backwards to find the start of the block! Note: If we don't
					// find the block, we keep 'index' at its original position so that we can quickly
					// exclude addresses that are outside of the rage we're interested in.
					size_t i = index;
					while (i > 0) {
						i--;

						// This is the start of a new block, use that!
						if (bStart[i] != 0xFF) {
							index = i;
							break;
						}
					}
				}


				// At this point, 'index' indicates the index of the first page group that may be
				// interesting to us. However, 'bStart[index]' may be 0xFF in some cases.

				// Fix that by finding a bStart that has something other than 0xFF, so that we find a
				// block that interests us!
				while (bStart[index] == 0xFF) {
					// If this is true, no addresses refer to any valid blocks since we didn't find any
					// block-starts in the indicated range.
					if (index == lastIndex)
						return;

					index++;
				}

				// Now, we have at least one block! Traverse adjacent blocks as far as possible and mark
				// them if we need to.
				page = index*wordBits + bStart[index];
				while (page <= lastPage) {
					index = page / wordBits;
					size_t offset = page % wordBits;

					if ((c.header()->used[index] >> offset) & 0x1) {
						// This block is in use (should always be true the first iteration, maybe not on subsequent ones).
						Block *block = (Block *)pageToAddr(c.header(), page);
						size_t blockSize = sizeof(Block) + block->size;
						size_t blockStart = size_t(block);
						size_t blockEnd = size_t(block) + blockSize;

						// Skip any entries in the list that are before the start of this block.
						while (size_t(*begin) < blockStart) {
							if (++begin >= end)
								return;
						}

						// If the current pointer is not too far off, we should mark the block!
						if (size_t(*begin) < blockEnd) {
							block->flags |= Block::fUpdated;

							// Skip any pointers inside the block.
							while (size_t(*begin) < blockEnd) {
								if (++begin >= end)
									return;
							}
						}

						// Jump to the next block.
						page += blockSize / pageSize;
					} else {
						// Try to find the next block.
						if (bStart[index] == 0xFF) {
							// Empty block?
							page = (index + 1) * wordBits;
						} else if (offset == wordBits - 1) {
							// Go to the next index, use the block-start table to skip some indices.
							// Note: There is never a tight fit after the block-start table due to how
							// the size of blocks are computed.
							offset = bStart[index + 1];
							if (offset == 0xFF) {
								// Skip the entire next block...
								page = (index + 2) * wordBits;
							} else {
								// Go to the proper position directly.
								page = (index + 1) * wordBits + offset;
							}
						} else {
							// Just go on until we find something more.
							page++;
						}
					}

					// If the next block is far enough away, it is beneficial to try to jump directly there.
					if ((index + 4) * wordBits < addrToPageSafe(c.header(), *begin))
						break;
				}

				// If we hit the end-condition of the previous loop, we're done.
				if (page > lastPage)
					break;
			}

			// If there are any pointers left here, they all refer to memory after the last block in this chunk.
		}

	}
}

#endif
