#include "stdafx.h"
#include "BlockAlloc.h"

#if STORM_GC == STORM_GC_SMM

#include "Gc/Gc.h"

namespace storm {
	namespace smm {

		/**
		 * The header of a chunk for convenient access.
		 */
		struct BlockAlloc::ChunkHeader {
			// Number of free pages.
			size_t freePages;

			// Location of the next potential allocation. We're currently using a "round-robin"
			// allocation strategy.
			size_t nextAlloc;

			// The actual data. One bit per page.
			size_t data[1];
		};


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
				vm->free(chunks[i].at, chunks[i].size);

			delete vm;
		}

		size_t BlockAlloc::headerSize(size_t size) {
			size_t pages = (size + pageSize - 1) / pageSize;
			size_t bytes = (pages + CHAR_BIT - 1) / CHAR_BIT;
			size_t total = sizeof(ChunkHeader) - sizeof(size_t) + bytes;
			return roundUp(total, pageSize);
		}

		void BlockAlloc::addChunk(void *mem, size_t size) {
			Chunk chunk(mem, size, headerSize(size));
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

		BlockAlloc::Chunk *BlockAlloc::findChunk(size_t size) {
			for (size_t i = 0; i < chunks.size(); i++) {
				Chunk &c = chunks[i];
				if (c.header()->freePages * pageSize >= size)
					return &c;
			}

			// TODO: Reserve more memory!
			assert(false, L"TODO: Reserve more memory!");
			return null;
		}

		Block *BlockAlloc::alloc(size_t size) {
			Chunk *c = findChunk(size);

			// TODO: Finish the implementation!

			return null;
		}

	}
}

#endif
