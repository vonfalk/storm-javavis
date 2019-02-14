#pragma once

#if STORM_GC == STORM_GC_SMM

#include "VM.h"
#include "Utils/Bitwise.h"
#include <vector>

namespace storm {
	namespace smm {

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
			 */
			struct Chunk {
				// Create.
				Chunk(void *at, size_t size, size_t header) : at(at), size(size), header(header) {}

				// The region itself.
				void *at;
				size_t size;

				// Offset of the header, so we don't have to compute it all the time.
				size_t header;
			};

			/**
			 * The header of a chunk for convenient access.
			 */
			struct ChunkHeader {
				// Location of the next potential allocation. We're currently using a "round-robin"
				// allocation strategy.
				size_t nextAlloc;

				// The actual data. One bit per page.
				size_t data[1];
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
		};

	}
}

#endif
