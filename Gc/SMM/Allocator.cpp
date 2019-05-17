#include "stdafx.h"
#include "Allocator.h"

#if STORM_GC == STORM_GC_SMM

#include "Generation.h"
#include "ArenaTicket.cpp"

namespace storm {
	namespace smm {

		// TODO: Is the large allocation limit reasonable?
		Allocator::Allocator(Generation &gen) : owner(gen), source(null), largeLimit(gen.blockSize / 4) {}

		Allocator::~Allocator() {
			if (source) {
				owner.arena.enter(owner, &Generation::done, source);
			}
		}

		void Allocator::fill(size_t minSize) {
			owner.arena.enter(*this, &Allocator::fillI, minSize);
		}

		void Allocator::fillI(ArenaTicket &e, size_t minSize) {
			if (source) {
				owner.done(e, source);
				source = null;
			}

			source = owner.alloc(e, minSize);

			// Our "large allocation" limit should make this impossible.
			assert(source);
			assert(source->size >= minSize);
		}

		PendingAlloc Allocator::allocLarge(size_t size) {
			// TODO: Allocate into a shared buffer inside a higher generation, rather than a single
			// large buffer. This requires using a lock for that generation's buffer, since we share
			// it with others.

			Generation *into = owner.next;
			if (!into)
				into = &owner;

			into->sharedBlockLock.lock();
			Block *shared = owner.arena.enter(owner, &Generation::sharedBlock, size);
			return PendingAlloc(shared, size, &into->sharedBlockLock);
		}

	}
}

#endif
