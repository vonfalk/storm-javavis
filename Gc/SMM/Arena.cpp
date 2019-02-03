#include "stdafx.h"
#include "Arena.h"

#if STORM_GC == STORM_GC_SMM

#include "Utils/Bitwise.h"
#include "Generation.h"
#include "Block.h"

namespace storm {
	namespace smm {

		// TODO: Make the nursery generation size customizable.
		Arena::Arena(size_t initialSize, const size_t *genSize, size_t generationCount)
			: generationCount(generationCount) {

			vm = VM::create(initialSize);

			// Allocate the individual generations.
			generations = (Generation *)malloc(generationCount * sizeof(Generation));

			for (size_t i = 0; i < generationCount; i++) {
				size_t genSz = roundUp(genSize[i], vm->allocGranularity);
				new (&generations[i]) Generation(*this, genSz);
				if (i + 1 < generationCount)
					generations[i].next = &generations[i + 1];
			}

			// TODO: If we need to be able to tell if some pointer may point inside a buffer, it is
			// probably a good idea to reserve a fair amount of virtual memory and then allocate
			// from there.
		}

		Arena::~Arena() {
			for (size_t i = 0; i < generationCount; i++)
				generations[i].~Generation();
			::free(generations);
		}

		Block *Arena::allocMin(size_t size) {
			size_t actual = sizeof(Block) + size;

			// Make sure we leave no holes!
			actual = roundUp(actual, vm->allocGranularity);
			void *mem = vm->alloc(actual);
			return new (mem) Block(actual - sizeof(Block));
		}

		void Arena::free(Block *block) {
			vm->free(block, sizeof(Block) + block->size);
		}

	}
}

#endif
