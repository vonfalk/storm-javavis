#pragma once

#if STORM_GC == STORM_GC_SMM

#include "VM.h"
#include "Utils/Bitwise.h"
#include "AddrSet.h"
#include <vector>

namespace storm {
	namespace smm {

		class Block;

		// Header of a chunk.
		struct ChunkHeader;

		/**
		 * Allocation management of blocks.
		 *
		 * Requests fairly large chunks of memory from the operating system and allocates them into
		 * fairly sized blocks that are suitable to use in generations and by allocation points,
		 * etc. In addition to raw memory management, this class provides information about the
		 * allocated memory. For example, it is possible to see if a particular address is managed
		 * by this allocator, and if the particular address is currently in use. Furthermore, the
		 * implementation attempts to keep the allocated memory close together, so that a fairly
		 * small AddrSet can be used to summarize pointers between pools, to further improve the
		 * efficiency of garbage collection.
		 */
		class BlockAlloc {
		public:
			// Create, with a backing VM instance. Making an initial reservation of approx. 'init' bytes.
			BlockAlloc(VM *backend, size_t initialSize);

			// Destroy.
			~BlockAlloc();

			// Our copy of the page size (we need it very often).
			const size_t pageSize;

			// Allocate a block, containing a minimum of 'size' bytes (excluding the header).
			Block *alloc(size_t size);

			// Free a previously allocated block.
			void free(Block *block);

			// Check for writes to any blocks we manage. Some backends only update the 'fUpdated'
			// flag for blocks this function is called, others do it during the actual write.
			void checkWrites(void **arenaBuffer);

			// Notify 'block' whenever it is written to.
			void watchWrites(Block *block);

			// Check if a pointer refers to an object managed by this instance. Returns 'true' if
			// the block is in the range reserved by any of the chunks here. This does not
			// necessarily mean that the pointer is actually allocated currently.
			inline bool has(void *addr) {
				size_t test = size_t(addr);
				for (size_t i = 0; i < chunks.size(); i++) {
					Chunk &c = chunks[i];
					size_t base = size_t(c.at);
					if (test >= base && test < base + c.pages * pageSize)
						return true;
				}
				return false;
			}

			// Create an AddrSet that is large enough to contain addresses in the entire arena
			// managed by this instance.
			template <class AddrSet>
			AddrSet addrSet() const {
				return AddrSet(minAddr, maxAddr);
			}

			// Find the block containing an address inside here. Fairly efficient, needs to traverse
			// a maximum of sizeof(size_t) * CHAR_BIT (ie. 32 or 64) blocks to find the correct one.
			Block *findBlock(void *addr);

			// Mark all blocks in the contained addresses as 'updated'. Assumes that the buffer is
			// sorted, so that the internal structures can be traversed more efficiently compared to
			// calling 'findBlock' for every address. Any addresses not currently referring to a
			// block are simply ignored.
			void markBlocks(void **addr, size_t count);


			/**
			 * A chunk of memory inside the BlockAlloc.
			 *
			 * Contains a pointer to an allocated region of memory and its size. The actual allocation
			 * starts with a bitmap that describes which pages in the allocation are currently used.
			 *
			 * Most of the functions that manipulate a chunk are inside BlockAlloc, as they also
			 * need access to the VM instance in use.
			 *
			 * Note: Keep this one as small as possible. We're searching through it inside 'has',
			 * which is on the hot path of the GC. Ideally, most things will be stored in the header
			 * itself.
			 */
			struct Chunk {
				// Create.
				Chunk(void *at, size_t pages) : at(at), pages(pages) {}

				// The region itself.
				void *at;

				// Size, expressed in number of pages.
				size_t pages;

				// Get the header.
				inline ChunkHeader *header() const { return (ChunkHeader *)at; }
			};

			// Get all chunks in this allocator. Mainly intended for VM subclasses.
			const vector<Chunk> &chunkList() const { return chunks; }

		private:
			// No copy!
			BlockAlloc(const BlockAlloc &o);
			BlockAlloc &operator =(const BlockAlloc &o);

			// VM backend.
			VM *vm;

			// Keep track of all chunks. We strive to keep this array small. Otherwise, the
			// "contains pointer" query will be expensive.
			vector<Chunk> chunks;

			// Min- and max address managed in this instance.
			size_t minAddr;
			size_t maxAddr;

			// Update 'minAddr' and 'maxAddr'.
			void updateMinMax();

			// Convert from a page index inside a chunk to an address.
			void *pageToAddr(ChunkHeader *header, size_t offset) const;

			// Convert from a pointer to a page index inside a chunk.
			size_t addrToPage(ChunkHeader *header, void *addr) const;

			// Same as 'addrToPage', but supports addresses inside the chunk header as well (returns
			// page 0 for them). Is slightly more expensive than 'addrToPage' for this reason.
			size_t addrToPageSafe(ChunkHeader *header, void *addr) const;

			// Add a chunk we recently reserved. This will initialize the allocation bitmap, and any
			// other data structures in the chunk, and finally add it to 'chunks'.
			void addChunk(void *mem, size_t size);

			// Find the chunk that contains a particular address.
			Chunk *findChunk(void *addr);

			// Allocate memory in a specific chunk. Returns 'null' on failure.
			Block *alloc(Chunk &c, size_t pages);

			// Mark blocks inside one particular chunk.
			void markBlocks(Chunk &c, void **begin, void **end);
		};

	}
}

#endif
