#include "stdafx.h"
#include "Arena.h"

#if STORM_GC == STORM_GC_SMM

#include "Utils/Bitwise.h"
#include "Generation.h"
#include "Block.h"

namespace storm {
	namespace smm {

		// Creates an ArenaEntry and initializes it properly.
#define ARENA_ENTRY(name) ArenaEntry name(*this); setjmp(name.buf)

		// TODO: Make the nursery generation size customizable.
		Arena::Arena(size_t initialSize, const size_t *genSize, size_t generationCount)
			: generationCount(generationCount), alloc(VM::create(), initialSize) {

			// Allocate the individual generations.
			generations = (Generation *)malloc(generationCount * sizeof(Generation));

			for (size_t i = 0; i < generationCount; i++) {
				size_t genSz = roundUp(genSize[i], alloc.pageSize);
				new (&generations[i]) Generation(*this, genSz);
				if (i + 1 < generationCount)
					generations[i].next = &generations[i + 1];
			}

			// For testing...
			PVAR(alloc.alloc(1024, 10));
			PVAR(alloc.alloc(1024, 10));
			PVAR(alloc.alloc(1024, 10));
			PVAR(alloc.alloc(1024, 10));
			PVAR(alloc.alloc(1024, 10));
			PVAR(alloc.alloc(1024, 10));
			PVAR(alloc.alloc(1024, 10));
			exit(1);
		}

		Arena::~Arena() {
			for (size_t i = 0; i < generationCount; i++)
				generations[i].~Generation();
			::free(generations);
		}

		Chunk Arena::allocChunk(size_t size, byte identifier) {
			util::Lock::L z(lock);
			return alloc.alloc(size, identifier);
		}

		void Arena::freeChunk(Chunk chunk) {
			util::Lock::L z(lock);
			alloc.free(chunk);
		}

		Thread *Arena::attachThread() {
			Thread *t = new Thread(*this);
			{
				util::Lock::L z(lock);
				threads.insert(t);
			}
			return t;
		}

		void Arena::detachThread(Thread *thread) {
			util::Lock::L z(lock);
			threads.erase(thread);
		}

		void Arena::collect() {
			ARENA_ENTRY(entry);

			// We're just testing stack scanning for the moment...
			for (size_t i = generationCount; i > 0; i--)
				generations[i - 1].collect(entry);
		}

	}
}

#endif
