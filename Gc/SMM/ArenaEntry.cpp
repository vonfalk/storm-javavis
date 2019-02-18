#include "stdafx.h"
#include "ArenaEntry.h"

#if STORM_GC == STORM_GC_SMM

#include "Arena.h"

namespace storm {
	namespace smm {

		ArenaEntry::ArenaEntry(Arena &owner) : owner(owner), lock(owner.lock) {}

	}
}

#endif
