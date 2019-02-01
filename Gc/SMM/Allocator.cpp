#include "stdafx.h"
#include "Allocator.h"

#if STORM_GC == STORM_GC_SMM

#include "Arena.h"

namespace storm {
	namespace smm {

		Allocator::Allocator(Arena &arena) : owner(arena), source(null), largeLimit(arena.nurserySize / 4) {}

		void Allocator::fill(size_t minSize) {
			// We might need to do something more...
			source = owner.allocNursery();
		}

		PendingAlloc Allocator::allocLarge(size_t size) {
			TODO(L"We want to handle large allocations in a better way than this...");
			return PendingAlloc(owner.allocMin(size), size);
		}

	}
}

#endif
