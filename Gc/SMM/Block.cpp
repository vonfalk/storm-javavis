#include "stdafx.h"
#include "Block.h"
#include "Scanner.h"
#include "Arena.h"

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

		struct VerifyPtr {
			typedef int Result;
			typedef Arena *Source;

			Arena &arena;

			VerifyPtr(Arena *arena) : arena(*arena) {}

			bool fix1(void *ptr) const {
				return arena.has(ptr);
			}

			int fix2(void **ptr) const {
				// Check padding etc. if possible.
				FMT_CHECK_OBJ(*ptr);

				// Make sure the header is sensible by computing the size of the object.
				fmt::size(*ptr);

				return 0;
			}

			SCAN_FIX_HEADER
		};

		void Block::dbg_verify(Arena *arena) {
			// Note: We're working with client pointers for convenience.
			byte *at = (byte *)mem(fmt::headerSize);
			byte *end = (byte *)mem(commit + fmt::headerSize);

			bool finalizers = false;

			while (at < end) {
				// Validate object headers if we're able to.
				FMT_CHECK_OBJ(fmt::fromClient(at));

				finalizers |= fmt::hasFinalizer(at);

				byte *next = at + fmt::size(at);
				assert(next <= end, L"An object is larger than the allocated portion of a block!");

				// Check containing pointers if possible.
				if (arena)
					fmt::Scan<VerifyPtr>::objects(arena, at, next);

				at = next;
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
