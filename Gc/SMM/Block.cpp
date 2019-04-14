#include "stdafx.h"
#include "Block.h"

#if STORM_GC == STORM_GC_SMM

namespace storm {
	namespace smm {

		void Block::watchWrites() {
			// We need to clear the 'updated' flag first. If we clear it afterwards, we will
			// immediately activate it again since we write to ourselves.

			TODO(L"Fixme!");
			// atomicAnd(flags, ~fUpdated);
			// inside->watchWrites(this);
		}

	}
}

#endif
