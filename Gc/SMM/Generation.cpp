#include "stdafx.h"
#include "Generation.h"

#if STORM_GC == STORM_GC_SMM

#include "Arena.h"
#include "Scanner.h"
#include "ObjWalk.h"

namespace storm {
	namespace smm {

		// TODO: What is a reasonable block size here?
		Generation::Generation(Arena &owner, size_t size)
			: totalSize(size), blockSize(size / 32),
			  next(null), owner(owner), sharedBlock(null) {}

		Generation::~Generation() {
			// Free all blocks we are in charge of.
			InlineSet<Block>::iterator i = blocks.begin();
			while (i != blocks.end()) {
				Block *remove = *i;
				++i;
				blocks.erase(remove);
			}
		}

		Block *Generation::alloc() {
			util::Lock::L z(lock);
			return allocBlock(blockSize);
		}

		void Generation::done(Block *block) {
			util::Lock::L z(lock);

			// TODO: Look at the amount of memory we're using, and consider triggering a garbage
			// collection, at least for this generation.
		}

		Block *Generation::fillBlock(size_t size) {
			if (sharedBlock && sharedBlock->remaining() < size) {
				done(sharedBlock);
				sharedBlock = null;
			}

			if (!sharedBlock)
				sharedBlock = allocBlock(max(size, blockSize));

			return sharedBlock;
		}

		Block *Generation::allocBlock(size_t size) {
			Block *b = owner.allocMin(size);
			if (b)
				blocks.insert(b);

			return b;
		}

		struct ClearMark {
			void operator() (fmt::Obj *obj) const {
				fmt::objClearMark(obj);
			}
		};

		struct MarkPinned {
			MarkPinned(PinnedSet pinned) : pinned(pinned) {}

			PinnedSet pinned;

			void operator() (fmt::Obj *obj) const {
				size_t size = fmt::objSize(obj);
				void *start = fmt::toClient(obj);
				if (pinned.has(start, (byte *)start + size)) {
					// Mark the object, and its descendants in this generation. TODO: We might want
					// to copy the objects right away.
					objWalk<128>(obj);
				}
			}
		};

		void Generation::collect(ArenaEntry &entry) {
			// First, clear any marked bits for all blocks.
			traverse(ClearMark());

			// Then, scan the roots and see which refer to the block. Note: We might want to scan
			// the stacks once and get pinned objects to all blocks we intend to scan at once. That
			// requires some kind of dynamic structure (which may be allocated in the block we intend
			// to copy to), that I am to lazy to implement at the moment.
			for (InlineSet<Block>::iterator i = blocks.begin(), end = blocks.end(); i != end; ++i) {
				Block *b = *i;

				PinnedSet pinned = b->addrSet<PinnedSet>();
				entry.scanStackRoots<ScanSummary<PinnedSet>>(pinned);

				// Mark pinned objects as reachable.
				b->traverse(MarkPinned(pinned));
			}

			// TODO: Mark all objects indirectly reachable (perhaps even copy them directly).
		}

	}
}

#endif
