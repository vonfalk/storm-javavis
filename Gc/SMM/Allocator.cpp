#include "stdafx.h"
#include "Allocator.h"

#if STORM_GC == STORM_GC_SMM

#include "Arena.h"

namespace storm {
	namespace smm {

		// TODO: Is the large allocation limit reasonable?
		Allocator::Allocator(Arena &arena) : owner(arena), source(null), largeLimit(arena.nurserySize / 4) {}

		void Allocator::fill(size_t minSize) {
			// We might need to do something more...
			source = owner.allocNursery();

			// Our "large allocation" limit should make this impossible.
			assert(source->size >= minSize);
		}

		PendingAlloc Allocator::allocLarge(size_t size) {
			// TODO: Allocate into a shared buffer inside a higher generation, rather than a single
			// large buffer. This requires using a lock for that generation's buffer, since we share
			// it with others.

			util::Lock *l = null;
			Block *b = owner.allocMin(size);
			return PendingAlloc(b, size, l);
		}

	}
}

#endif
