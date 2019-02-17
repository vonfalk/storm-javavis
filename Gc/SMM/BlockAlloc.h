#pragma once

#if STORM_GC == STORM_GC_SMM

#include "VM.h"
#include "Block.h"
#include "Utils/Bitwise.h"
#include "AddrSet.h"
#include <vector>

namespace storm {
	namespace smm {

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

		private:
			// No copy!
			BlockAlloc(const BlockAlloc &o);
			BlockAlloc &operator =(const BlockAlloc &o);

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


			// VM backend.
			VM *vm;

			// Keep track of all chunks. We strive to keep this array small. Otherwise, the
			// "contains pointer" query will be expensive.
			vector<Chunk> chunks;

			// Compute the size of a chunk's header (rounded up to the next page boundary).
			size_t headerSize(size_t size);

			// Add a chunk we recently reserved. This will initialize the allocation bitmap, and any
			// other data structures in the chunk, and finally add it to 'chunks'.
			void addChunk(void *mem, size_t size);

			// Find the chunk that contains a particular address.
			Chunk *findChunk(void *addr);

			// Allocate memory in a specific chunk. Returns 'null' on failure.
			Block *alloc(Chunk &c, size_t pages);
		};

	}
}

#endif
