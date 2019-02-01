#include "stdafx.h"
#include "Arena.h"

#if STORM_GC == STORM_GC_SMM

#include "Utils/Bitwise.h"
#include "Block.h"

namespace storm {
	namespace smm {

		// TODO: Make the nursery generation size customizable.
		Arena::Arena(size_t initialSize) : nurserySize(100*1024) {
			allocGranularity = vmAllocGranularity();
			pageSize = vmPageSize();

			// TODO: If we need to be able to tell if some pointer may point inside a buffer, it is
			// probably a good idea to reserve a fair amount of virtual memory and then allocate
			// from there.
		}

		Arena::~Arena() {}

		Block *Arena::allocMin(size_t size) {
			size_t actual = sizeof(Block) + size;

			// Make sure we leave no holes!
			actual = roundUp(actual, allocGranularity);
			void *mem = vmAlloc(actual);
			return new (mem) Block(actual - sizeof(Block));
		}

		void Arena::free(Block *block) {
			vmFree(block, sizeof(Block) + block->size);
		}

		Block *Arena::allocNursery() {
			return allocMin(nurserySize);
		}

	}
}

#endif
