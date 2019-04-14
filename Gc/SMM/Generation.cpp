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
			// Block *b = owner.allocMin(size);
			// if (b)
			// 	blocks.insert(b);

			// return b;
			return null;
		}

		struct ClearMark {
			void operator() (fmt::Obj *obj) const {
				fmt::objClearMark(obj);
			}
		};

		/**
		 * Move all objects to a new generation. Pinned objects will not be moved.
		 */
		struct MoveObj {
			MoveObj(PinnedSet pinned, Generation *to) : pinned(pinned), to(to) {}

			PinnedSet pinned;
			Generation *to;

			void operator() (fmt::Obj *obj) {
				size_t size = fmt::objSize(obj);
				void *client = fmt::toClient(obj);

				// Don't move pinned objects.
				if (pinned.has(client, (byte *)client + size))
					return;

				// TODO: We probably want to optimize this at least a tiny bit by batching calls to 'fillBlock'.
				// Note: We don't take the lock as we assume we're collecting in a single threaded fashion.
				// For this reason, we can also ignore much of the allocation protocol, and just update 'committed'
				// and 'reserved' directly.
				Block *into = to->fillBlock(size);
				fmt::Obj *newPos = (fmt::Obj *)into->mem(into->committed);
				memcpy(newPos, obj, size);
				into->committed += size;
				into->reserved = into->committed;

				// Remember that we forwarded the object, so that we can update pointers later!
				fmt::objMakeFwd(obj, size, fmt::toClient(newPos));
			}
		};

		/**
		 * Mark objects inside the specified block.
		 */
		struct MarkInBlock {
			typedef Block *Source;
			typedef int Result;

			size_t from, to;

			MarkInBlock(Block *block) : from(size_t(block->mem(0))), to(size_t(block->mem(block->committed))) {}

			bool fix1(void *ptr) const {
				size_t p = size_t(ptr);
				return (p >= from) & (p < to);
			}

			Result fix2(void **ptr) const {
				fmt::objSetMark(fmt::fromClient(*ptr));
				return 0;
			}

			SCAN_FIX_HEADER
		};

		/**
		 * Mark and move pinned objects.
		 */
		struct WalkPinned {
			WalkPinned(PinnedSet pinned, Generation *to, Generation::Contains pred)
				: move(pinned, to), predicate(pred) {}

			MoveObj move;
			Generation::Contains predicate;

			void operator() (fmt::Obj *obj) {
				size_t size = fmt::objSize(obj);
				void *start = fmt::toClient(obj);
				if (move.pinned.has(start, (byte *)start + size)) {
					// Mark the object and its descendants in this generation, and move the ones
					// not pinned.
					objWalk<Generation::Contains, MoveObj, 128>(obj, predicate, move);
				}
			}
		};


		void Generation::collect(ArenaEntry &entry) {
			// Copy objects into the next generation if it exists, otherwise just into this generation.
			Generation *nextGen = next;
			if (!nextGen)
				nextGen = this;

			// We only concern ourselves with these objects.
			Contains predicate = containsPredicate();

			// This remains fixed, even if 'blocks' is changed.
			InlineSet<Block>::iterator end = blocks.end();

			// First, clear any marked bits for all blocks.
			walk(ClearMark());

			// Then, scan the roots and see which refer to the block. Note: We might want to scan
			// the stacks once and get pinned objects to all blocks we intend to scan at once. That
			// requires some kind of dynamic structure (which may be allocated in the block we intend
			// to copy to), that I am to lazy to implement at the moment.
			for (InlineSet<Block>::iterator i = blocks.begin(); i != end; ++i) {
				Block *b = *i;

				// Mark any objects in this generation from other generations.
				entry.scanGenerations<MarkInBlock>(b, this, b);

				PinnedSet pinned = b->addrSet<PinnedSet>();
				entry.scanStackRoots<ScanSummary<PinnedSet>>(pinned);

				// Mark pinned objects as reachable and scan from there.
				// TODO: Which generations do we want to walk?
				b->walk(WalkPinned(pinned, nextGen, predicate));
			}

			// Fill any unmarked objects with forwarders to NULL so that weak pointers get splatted
			// when we update pointer in a moment.
			for (InlineSet<Block>::iterator i = blocks.begin(); i != end; ++i) {
				Block *b = *i;

				if (!b->sweep())
					b->flags |= Block::fEmpty;
				else
					b->flags |= Block::fSwept;
			}

			// Update pointers in objects that referred to the objects we moved.
			for (InlineSet<Block>::iterator i = blocks.begin(); i != end; ++i) {
				entry.scanGenerations<UpdateFwd<Contains>>(predicate, this, *i);
				if (((*i)->flags & Block::fEmpty) == 0)
					(*i)->scan<UpdateFwd<Contains>>(predicate);
			}

			// Also scan the new blocks!
			if (nextGen != this)
				nextGen->scan<UpdateFwd<Contains>>(predicate);

			// Reclaim empty blocks. TODO: Most of these will contain a fair amount of unused space
			// in the form of forwarding objects. It would be nice to be able to re-use that memory,
			// especially when we're dealing with blocks containing non-moving objects.
			{
				InlineSet<Block>::iterator i = blocks.begin();
				while (i != end) {
					Block *b = *i;
					++i;

					// Note: Freeing the memory *after* the iterator goes past it.
					if (b->flags & Block::fEmpty) {
						blocks.erase(b);
						// owner.free(b);
					} else if (b->flags & Block::fSwept) {
						// Make the block more efficient to scan by merging adjacent garbage objects.
						b->clean();
						b->flags &= ~Block::fSwept;
					}
				}
			}
		}

	}
}

#endif