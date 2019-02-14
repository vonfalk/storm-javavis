#include "stdafx.h"
#include "BlockAlloc.h"

#if STORM_GC == STORM_GC_SMM

#include "Gc/Gc.h"

namespace storm {
	namespace smm {

		BlockAlloc::BlockAlloc(VM *vm, size_t initSize) : vm(vm) {
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
			size_t pageSz = vm->pageSize;
			size_t pages = (size + pageSz - 1) / pageSz;
			size_t bytes = (pages + CHAR_BIT - 1) / CHAR_BIT;
			size_t total = sizeof(ChunkHeader) - sizeof(size_t) + bytes;
			return roundUp(total, pageSz);
		}

		void BlockAlloc::addChunk(void *mem, size_t size) {
			size_t header = headerSize(size);

			// Commit the memory for the header. We don't need to initialize it to zero, the OS will
			// do this for us!
			vm->commit(mem, header);

			chunks.push_back(Chunk(mem, size, header));
		}

	}
}

#endif
