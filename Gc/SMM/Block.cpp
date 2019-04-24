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

		void Block::dbg_verify() {
			// Note: We're working with client pointers for convenience.
			byte *at = (byte *)mem(fmt::headerSize);
			byte *end = (byte *)mem(commit + fmt::headerSize);

			while (at < end) {
				// Validate the object if we're able to.
				FMT_CHECK_OBJ(fmt::fromClient(at));

				at += fmt::size(at);
				assert(at <= end, L"An object is larger than the allocated portion of a block!");
			}

			assert(at == end, L"Invalid allocation size in a block!");
		}

		void Block::dbg_dump() {
			PNN(L"Block at " << this);
			PNN(L", size " << std::setw(5) << size);
			PNN(L", committed " << std::setw(5) << committed());
			PNN(L", reserved " << std::setw(5) << reserved());
			PNN(L", next " << next());
			PLN(L", flags " << flags);
		}

	}
}

#endif
