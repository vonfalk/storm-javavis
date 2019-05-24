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

		void Block::fillSummary(MemorySummary &summary) {
			summary.bookkeeping += sizeof(Block);

			void *tmp;
			for (void *at = mem(fmt::headerSize); at != mem(committed() + fmt::headerSize); at = fmt::skip(at)) {
				size_t size = fmt::size(at);
				if (fmt::isFwd(at, &tmp)) {
					summary.bookkeeping += size;
				} else if (fmt::isPad(at)) {
					summary.fragmented += size;
				} else {
					summary.objects += size;
				}
			}

			summary.free += remaining();
		}

		void Block::dbg_verify() {
			// Note: We're working with client pointers for convenience.
			byte *at = (byte *)mem(fmt::headerSize);
			byte *end = (byte *)mem(commit + fmt::headerSize);

			bool finalizers = false;

			while (at < end) {
				// Validate the object if we're able to.
				FMT_CHECK_OBJ(fmt::fromClient(at));

				finalizers |= fmt::hasFinalizer(at);

				at += fmt::size(at);
				assert(at <= end, L"An object is larger than the allocated portion of a block!");
			}

			assert(hasFlag(Block::fFinalizers) || !finalizers,
				L"The block " + ::toHex(this) + L" contains finalizers without the finalizer flag being set!");
			assert(at == end, L"Invalid allocation size in a block!");
		}

		void Block::dbg_dump() {
			PNN(L"Block at " << this);
			PNN(L", size " << std::setw(5) << (void *)size);
			PNN(L", committed " << std::setw(5) << (void *)committed());
			PNN(L", reserved " << std::setw(5) << (void *)reserved());
			PNN(L", next " << next());
			PLN(L", flags " << flags);
		}

	}
}

#endif
