#include "stdafx.h"
#include "Block.h"

#if STORM_GC == STORM_GC_SMM

namespace storm {
	namespace smm {

		bool Block::sweep() {
			bool any = false;

			fmt::Obj *end = (fmt::Obj *)mem(committed);
			fmt::Obj *at = (fmt::Obj *)mem(0);
			while (at != end) {
				size_t size = fmt::objSize(at);

				bool marked = fmt::objIsMarked(at);
				any |= marked;

				if (!marked & !fmt::objIsSpecial(at))
					fmt::objMakeFwd(at, size, null);

				void *next = (byte *)at + size;
				at = (fmt::Obj *)next;
			}

			return any;
		}

		void Block::clean() {
			fmt::Obj *end = (fmt::Obj *)mem(committed);
			fmt::Obj *at = (fmt::Obj *)mem(0);

			// First object we might be able to destroy.
			fmt::Obj *free = at;
			while (at != end) {
				fmt::Obj *next = fmt::objSkip(at);

				if (!fmt::objIsSpecial(at)) {
					// We need to preserve this one.
					free = next;
				} else {
					// Merge any previous object with this one.
					fmt::objMakePad(free, (byte *)next - (byte *)free);
				}

				at = next;
			}

			// Ignore the trailing pad objects. We don't need to scan them anyway.
			reserved = committed = (byte *)free - (byte *)mem(0);
		}

		void Block::watchWrites() {
			// We need to clear the 'updated' flag first. If we clear it afterwards, we will
			// immediately activate it again since we write to ourselves.
			atomicAnd(flags, ~fUpdated);
			// inside->watchWrites(this);
		}

	}
}

#endif
