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

			// Check assumptions of the formatting code.
			fmt::init();

			// Check our limit on generations. If this is broken, things will go very wrong.
			assert(generationCount < (1 << 6), L"Must have less than 2^6 generations.");

			// Allocate the individual generations.
			generations = (Generation *)malloc(generationCount * sizeof(Generation));

			// TODO: We most likely want to connect the generations in a more interesting pattern,
			// e.g. duplicating the last generation and connecting them in a cycle to avoid special
			// cases for the last generation. Additionally, it is worth investigating whether it is
			// useful to have parallel "lanes" for objects of certain types. E.g. objects with
			// finalizers.

			for (size_t i = 0; i < generationCount; i++) {
				size_t genSz = roundUp(genSize[i], alloc.pageSize);
				new (&generations[i]) Generation(*this, genSz, i + 1);
				if (i + 1 < generationCount)
					generations[i].next = &generations[i + 1];
			}
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

			// The user program asked us for a full collection, causing all generations to be
			// collected (not simultaneously at the moment, we might want to do that in order to
			// reduce memory usage as much as possible without requiring multiple calls to 'collect').
			for (size_t i = generationCount; i > 0; i--)
				generations[i - 1].collect(entry);
		}

	}
}

#endif
