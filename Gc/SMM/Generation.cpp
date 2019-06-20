#include "stdafx.h"
#include "Generation.h"

#if STORM_GC == STORM_GC_SMM

#include "Arena.h"
#include "Scanner.h"
#include "ScanState.h"
#include "Thread.h"
#include "Nonmoving.h"
#include "ArenaTicket.h"
#include "FinalizerPool.h"
#include "UpdateFwd.h"
#include "Util.h"

namespace storm {
	namespace smm {

		// TODO: What is a reasonable block size here?
		Generation::Generation(Arena &arena, size_t size, byte identifier)
			: totalSize(size), blockSize(size / 32),
			  next(null), arena(arena), identifier(identifier),
			  lastChunk(0), totalAllocBytes(0), totalFreeBytes(0), shared(null) {

			blockSize = min(size_t(64 * 1024), blockSize);
			pinnedSets.push_back(PinnedSet(0, 1));
		}

		Generation::~Generation() {
			// Free all blocks we are in charge of.
			for (size_t i = 0; i < chunks.size(); i++) {
				arena.freeChunk(chunks[i].memory);
			}
		}

		Block *Generation::alloc(ArenaTicket &ticket, size_t minSize) {
			return allocBlock(ticket, minSize, blockSize);
		}

		void Generation::done(ArenaTicket &ticket, Block *block) {
			block->reserved(block->committed());

			ChunkList::iterator pos = std::lower_bound(chunks.begin(), chunks.end(), block, PtrCompare());
			if (pos != chunks.end()) {
				totalFreeBytes -= pos->freeBytes;

				pos->releaseBlock(block);

				totalFreeBytes += pos->freeBytes;
			} else {
				dbg_assert(false, L"Calling 'done' for a block not belonging to this generation!");
			}

			// If we're using more memory than we're allowed, trigger a collection of us!
			if (totalAllocBytes > totalSize)
				ticket.scheduleCollection(this);
		}

		bool Generation::isPinned(void *obj, void *end) {
			ChunkList::iterator pos = std::lower_bound(chunks.begin(), chunks.end(), obj, PtrCompare());
			if (pos != chunks.end()) {
				return pinnedSets[pos - chunks.begin()].has(obj, end);
			} else {
				// Should not happen...
				return false;
			}
		}

		Block *Generation::sharedBlock(ArenaTicket &ticket, size_t size) {
			if (shared && shared->remaining() < size) {
				done(ticket, shared);
				shared = null;
			}

			if (!shared)
				shared = allocBlock(ticket, size, max(size + (size >> 1), blockSize));

			return shared;
		}

		Block *Generation::allocBlock(ArenaTicket &ticket, size_t minSize, size_t maxSize) {
			if (minSize > maxSize)
				return null;

			// Find a chunk where the allocation may fit.
			if (Block *r = findFreeBlock(minSize, maxSize))
				return r;

			// Try to do collect this generation!
			if (ticket.suggestCollection(this)) {
				// Success! There might be more to gain!
				if (Block *r = findFreeBlock(minSize, maxSize))
					return r;
			} else if (totalAllocBytes >= totalSize) {
				// If we're using too much memory, we definitely need to be collected!
				ticket.scheduleCollection(this);
			}


			// We need to allocate more memory. TODO: How much?
			Chunk c = arena.allocChunk(max(minSize, defaultChunkSize()), identifier);

			// Out of memory?
			if (c.empty())
				return null;

			// Insert the new chunk at the proper location.
			size_t pos = insertSorted(chunks, GenChunk(c), ChunkCompare());
			lastChunk = pos;

			// TODO: We might want to merge blocks, but that requires us to be able to split and
			// deallocate unused blocks as well...

			while (pinnedSets.size() < chunks.size() + 1)
				pinnedSets.push_back(PinnedSet(0, 1));

			totalAllocBytes += c.size;
			Block *r = chunks[pos].allocBlock(minSize, maxSize, minFragment());
			totalFreeBytes += chunks[pos].freeBytes;
			return r;
		}

		Block *Generation::findFreeBlock(size_t minSize, size_t maxSize) {
			if (minSize > maxSize)
				return null;

			if (lastChunk >= chunks.size())
				lastChunk = 0;

			// Find a chunk where the allocation may fit.
			for (size_t i = 0; i < chunks.size(); i++) {
				GenChunk &chunk = chunks[lastChunk];
				size_t chunkFree = chunk.freeBytes;

				if (Block *r = chunk.allocBlock(minSize, maxSize, minFragment())) {
					totalFreeBytes -= chunkFree;
					totalFreeBytes += chunk.freeBytes;
					return r;
				}

				if (++lastChunk >= chunks.size())
					lastChunk = 0;
			}

			return null;
		}

		void Generation::collect(ArenaTicket &ticket) {
			dbg_assert(next, L"Need a next generation for collection!");
			dbg_assert(next != this, L"We can't collect to ourselves!");

			// No need for GC if we're empty.
			if (chunks.empty())
				return;

			ticket.gcRunning();

			// We use mark and sweep for the non-moving objects.
			Nonmoving &nonmoving = ticket.nonmoving();

			// Note: This assumes a generation where objects may move. Non-moving objects need to be
			// treated differently (especially since we don't expect there to be very many of them).

			// Scan all non-exact roots (the stacks of all threads), and keep track of non-moving
			// objects.

			for (size_t i = 0; i < chunks.size(); i++)
				pinnedSets[i] = chunks[i].memory.addrSet<PinnedSet>();
			pinnedSets[chunks.size()] = nonmoving.addrSet<PinnedSet>();

			// TODO: We might want to do an 'early out' inside the scanning by using
			// VMAlloc::identifier before attempting to access the sets. We need to measure the benefits of this!
			ticket.scanInexactRoots<ScanSummaries<PinnedSet>>(pinnedSets);

			// Keep track of surviving objects inside a ScanState object, which allocates memory
			// from the next generation.
			ScanState state(ticket, State(*this), next);

			// Scanner to use when moving to the next generation.
			typedef ScanNonmoving<ScanState::Move, fmt::ScanAll, true> GenScanner;
			typedef ScanNonmoving<ScanState::Move, IfNotWeak, true> GenNoWeakScanner;

			// Scan the nonmoving objects first, then we can update the marks there at the same time!
			nonmoving.scanPinned<GenNoWeakScanner>(pinnedSets[chunks.size()], state);

			Block *finalizerBlocks = null;
			// For all blocks containing at least one pinned object, traverse it entirely to find
			// and scan the pinned objects. Any non-pinned objects are copied to the new block.
			for (size_t i = 0; i < chunks.size(); i++)
				chunks[i].scanPinnedFindFinalizers<IfNotWeak, GenNoWeakScanner>(IfNotWeak(), pinnedSets[i], state, finalizerBlocks);

			// Traverse all other generations that could contain references to this generation and
			// copy any referred objects to the new block.
			ticket.scanGenerations<IfNotWeak, GenNoWeakScanner>(IfNotWeak(), state, this);

			// Also traverse exact roots.
			ticket.scanExactRoots<GenNoWeakScanner>(state);

			// Traverse the newly copied objects in the new block and copy any new references until
			// no more objects are found.
			// Note: If we're sure that the scanned objects contain no references to themselves, we
			// can actually avoid this step entirely. We would, however, tell the ScanState to
			// release the held Blocks in this case.
			state.scanNew();

			// Grab objects referred to only by old finalizers and keep them alive in the finalizing
			// generation for the time being. We want to keep objects being finalized intact for as
			// long as possible, so that finalizers will see a fairly good view of the world.
			FinalizerPool &pool = ticket.finalizerPool();
			pool.scan<FinalizerPool::Move>(ticket, FinalizerPool::Move::Params(pool, State(*this)));

			// Check the blocks containing finalizers found before and grab all objects with
			// finalizers and put them inside the finalizer pool.
			for (Block *at = finalizerBlocks; at; at = at->next()) {
				ChunkList::iterator pos = std::lower_bound(chunks.begin(), chunks.end(), at, PtrCompare());
				dbg_assert(pos != chunks.end(), L"Could not find a pinned set!");
				at->traverse(FinalizerPool::MoveFinalizers(pool, pinnedSets[pos - chunks.begin()]));
			}

			// Recursively grab all dependencies of the objects we moved to the finalizer pool!
			pool.scanNew(ticket, State(*this));

			// Update any references to objects we just moved.

			// Note: We don't need to scan objects we have moved. The ScanState makes sure to update
			// all pointers when we have to scan them anyway! Therefore, we don't need to scan this
			// generation again. (Perhaps any objects that need finalization though).

			// Note: We do, however, need to scan any weak references we found, since they may
			// contain references that turned stale during scanning. The ScanState helpfully keeps
			// track of this for us, so this is fairly cheap.
			state.scanWeak<UpdateWeakFwd>(State(*this));

			// Note: We try to not scan the objects we moved to a new generation immediately at this
			// point. ScanState sets the flag fSkipScan on all blocks that were empty when they were
			// allocated, and 'scan' in the generations will then ignore scanning once if that flag
			// is set. We know that the ticket will not perform an early out and not check the next
			// generation here, because we just added new blocks to that generation, which requires
			// a good enough re-scan of the relevant blocks.
			// This avoids scanning some objects twice at least. This is improved if the generation
			// would be a bit more aggressive at splitting blocks during allocations, being more
			// likely to return new blocks even though the already existing ones are suitable.
			{
				State s(*this);
				MixedPredicate p(s);
				ticket.scanGenerations<MixedPredicate, UpdateMixedFwd>(p, p, this);
			}
			// ticket.scanGenerations<UpdateFwd>(State(*this), this);

			// Update exact roots.
			ticket.scanExactRoots<UpdateFwd>(State(*this));

			// Update any old finalizer references as well (we don't need to be careful with weak
			// references here, they'll be destroyed soon enough anyway!).
			pool.scan<UpdateFwd>(ticket, State(*this));

			// Sweep objects in the nonmoving pool, and update references while we're at it.
			nonmoving.scanSweep<UpdateFwd>(State(*this));

			// Finally, release and/or compact any remaining blocks in this generation.
			totalAllocBytes = 0;
			totalFreeBytes = 0;
			for (size_t i = 0; i < chunks.size(); i++) {
				GenChunk &chunk = chunks[i];
				PinnedSet &pinned = pinnedSets[i];

				if (chunk.compact(pinned)) {
					arena.freeChunk(chunk.memory);
					chunks.erase(chunks.begin() + i);
					i--;
				} else {
					totalAllocBytes += chunk.memory.size;
					totalFreeBytes += chunk.freeBytes;
					if (lastChunk >= i && lastChunk > 0)
						lastChunk--;
				}
			}
		}

		void Generation::runFinalizers() {
			for (size_t i = 0; i < chunks.size(); i++) {
				chunks[i].runFinalizers();
			}
		}

		void Generation::fillSummary(MemorySummary &summary) const {
			for (size_t i = 0; i < chunks.size(); i++) {
				summary.allocated += chunks[i].memory.size;
				chunks[i].fillSummary(summary);
			}
		}

		void Generation::dbg_verify() {
			assert(chunks.size() == pinnedSets.size());

			size_t alloc = 0;
			size_t free = 0;

			for (size_t i = 0; i < chunks.size(); i++) {
				if (i > 0) {
					assert((byte *)chunks[i - 1].memory.at < (byte *)chunks[i].memory.at, L"Unordered chunks!");
				}

				chunks[i].dbg_verify();

				alloc += chunks[i].memory.size;
				free += chunks[i].freeBytes;
			}

			assert(totalAllocBytes == alloc, L"The count of total allocated bytes is incorrect!");
			assert(totalFreeBytes == free, L"The count of total free bytes is incorrect!");
		}

		void Generation::dbg_dump() {
			PLN(L"Generation with id " << identifier << L", " << chunks.size() << L" chunks:");
			for (size_t i = 0; i < chunks.size(); i++) {
				PNN(i << L": ");
				chunks[i].dbg_dump();
			}
			PLN(L"Allocated: " << totalAllocBytes << L", used: " << totalUsedBytes() << L", free: " << totalFreeBytes);
		}


		/**
		 * A chunk.
		 */

		Generation::GenChunk::GenChunk(Chunk chunk) : memory(chunk) {
			lastAlloc = new (memory.at) Block(memory.size - sizeof(Block));
			freeBytes = memory.size - sizeof(Block);

			// TODO: We might want to use one of the free chunks to contain a data structure to
			// accelerate allocations! That memory is committed anyway, so we might as well use it.
		}

		static inline Block *next(Block *b) {
			return (Block *)b->mem(b->size);
		}

		static inline Block *nextWrap(Block *b, const Chunk &limit) {
			Block *n = next(b);
			if ((size_t)n - (size_t)limit.at >= limit.size)
				n = (Block *)limit.at;
			return n;
		}

		Block *Generation::GenChunk::allocBlock(size_t minSize, size_t maxSize, size_t minFragment) {
			// We will definitely fail to find a suitable allocation spot...
			if (freeBytes < minSize)
				return null;

			// Visit all elements in turn, starting at 'lastAlloc' and try to find something good!
			bool first = true;
			for (Block *at = lastAlloc; first || at != lastAlloc; at = nextWrap(at, memory), first = false) {
				// If it is used by an allocator, skip it.
				if (at->hasFlag(Block::fUsed))
					continue;

				// Not enough space?
				size_t remaining = at->remaining();
				if (remaining < minSize)
					continue;

				// Split the block?
				if (remaining > maxSize && remaining > minSize + minFragment + sizeof(Block)) {
					// Split at either the maximum size, or at the size that yeilds 'minFragment'
					// bytes in the other block.
					size_t splitAfter = min(maxSize, remaining - minFragment - sizeof(Block));
					size_t used = at->committed();
					bool finalizers = at->hasFlag(Block::fFinalizers);

					// Create the new block.
					new (at->mem(used + splitAfter)) Block(at->size - used - splitAfter - sizeof(Block));
					freeBytes -= sizeof(Block);

					// Re-create the old block to change its size.
					new (at) Block(used + splitAfter);
					at->committed(used);
					at->reserved(used);
					if (finalizers)
						at->setFlag(Block::fFinalizers);
				}

				// This is the one we want!
				at->setFlag(Block::fUsed);
				freeBytes -= at->remaining();
				lastAlloc = at;
				return at;
			}

			// No block found.
			return null;
		}

		void Generation::GenChunk::releaseBlock(Block *block) {
			block->clearFlag(Block::fUsed);
			freeBytes += block->remaining();
		}

		bool Generation::GenChunk::compact(const PinnedSet &pinned) {
			// The block that is currently being expanded.
			Block *current = (Block *)memory.at;

			// Reset internal data structures.
			lastAlloc = current;
			freeBytes = 0;

			// Traverse the blocks inside this chunk to find things we need to preserve. We need to
			// keep chunks marked as used and pinned objects. The rest is known to be free. We do,
			// however, need to consider finalizers.
			Block *at = (Block *)memory.at;
			while (at != (Block *)memory.end()) {
				// We need to read this first. Otherwise, we might destroy the old value when we're compacting!
				Block *next = (Block *)at->mem(at->size);

				if (at->hasFlag(Block::fUsed)) {
					// Used block, we need to keep this pointer valid and the size
					// constant. However, we are free to decrease 'committed' if we are able to!

					compactFinishBlock(current, at);
					shrinkBlock(at, pinned);

					// Skip ahead, don't touch the size of this block anymore.
					current = (Block *)at->mem(at->size);
				} else if (pinned.has(at->mem(0), at->mem(at->committed()))) {
					// Unused block with pinned objects. We need to preserve those...
					current = compactPinned(current, at, pinned);
				} else {
					// Unused block. We are free to remove this block entirely without looking at it anymore.
					at->committed(0);
					at->reserved(0);
				}

				at = next;
			}

			// Finish the last block if necessary.
			compactFinishBlock(current, (Block *)memory.end());

			// If the compaction resulted in a single empty block that is not marked as used, we're empty.
			Block *first = (Block *)memory.at;
			return first->mem(first->size) == memory.end()    // one block
				&& first->committed() == 0                    // empty
				&& !first->hasFlag(Block::fUsed);             // not in use
		}

		Block *Generation::GenChunk::compactFinishObj(Block *current, void *until) {
			// Usable size in 'current'. If we are unable to fit a second block header there, we
			// need to merge them into one large block instead!
			size_t size = (byte *)until - (byte *)current - sizeof(Block);
			size_t used = current->committed();
			bool finalizers = current->hasFlag(Block::fFinalizers);

			if (size < sizeof(Block) + used) {
				// One large object. Just add a padding object and continue!
				size_t padSz = size - used;
				if (padSz > 0)
					fmt::objMakePad((fmt::Obj *)current->mem(used), padSz);
				current->committed(size);
				current->reserved(size);
				return current;
			}

			// We are better off creating a new object, so that we can use the gap!
			size -= sizeof(Block);
			current = new (current) Block(size);
			current->committed(used);
			current->reserved(used);
			if (finalizers)
				current->setFlag(Block::fFinalizers);

			freeBytes += current->remaining();

			// Create a new block with a temporary size of zero. It will be recreated later with the
			// proper size, and we just need a header! Note: Block initializes 'committed' and
			// 'reserved' to zero.
			return new (current->mem(size)) Block(0);
		}

		void Generation::GenChunk::compactFinishBlock(Block *current, Block *next) {
			// If they are the same, just ignore the request.
			if (current == next)
				return;

			// Note: If we get past the previous check, we know that 'current' does not have the
			// 'used' flag set due to how 'compact' works. Thus, we can ignore the flags in the block.
			size_t size = (byte *)next - (byte *)current - sizeof(Block);
			size_t used = current->committed();
			bool finalizers = current->hasFlag(Block::fFinalizers);

			current = new (current) Block(size);
			current->committed(used);
			current->reserved(used);
			if (finalizers)
				current->setFlag(Block::fFinalizers);

			freeBytes += current->remaining();
		}

		Block *Generation::GenChunk::compactPinned(Block *current, Block *scan, const PinnedSet &pinned) {
			fmt::Obj *at = (fmt::Obj *)scan->mem(0);
			fmt::Obj *to = (fmt::Obj *)scan->mem(scan->committed());

			// If 'scan == current', this is vital. Otherwise 'compactFinishObj' will be confused
			// when we call it the first time and the current object is before the extend of the
			// current block.
			scan->committed(0);

			// We will check for finalizers while we scan the block. If it was previously set and
			// all finalizers are eliminated, it will remain set otherwise, which is wasteful.
			scan->clearFlag(Block::fFinalizers);

			// Keep track of any finalizers in this block.
			bool finalizers = false;

			// Find rising and falling 'edges' in the usage, and structure the blocks accordingly.
			bool lastInUse = false;

			while (at != to) {
				fmt::Obj *next = fmt::objSkip(at);
				// Note: We don't need to preserve padding objects, even if they happen to be pinned.
				bool inUse = pinned.has(fmt::toClient(at), next) & !fmt::objIsPad(at);

				finalizers |= inUse & fmt::objHasFinalizer(at);

				if (inUse & !lastInUse) {
					// First object in use. We might want to start a new block.
					current = compactFinishObj(current, at);
				} else if (!inUse & lastInUse) {
					// Last object in a hunk, mark the size accordingly.
					size_t sz = (byte *)at - (byte *)current->mem(0);
					current->committed(sz);
					current->reserved(sz);
					if (finalizers)
						current->setFlag(Block::fFinalizers);
				}

				lastInUse = inUse;
				at = next;
			}

			// If the last few objects were in use, mark the size accordingly.
			if (lastInUse) {
				size_t sz = (byte *)to - (byte *)current->mem(0);
				current->committed(sz);
				current->reserved(sz);
				if (finalizers)
					current->setFlag(Block::fFinalizers);
			}

			return current;
		}

		void Generation::GenChunk::shrinkBlock(Block *block, const PinnedSet &pinned) {
			fmt::Obj *at = (fmt::Obj *)block->mem(0);
			fmt::Obj *to = (fmt::Obj *)block->mem(block->committed());

			// First free location as far as we know.
			fmt::Obj *freeFrom = at;
			bool finalizers = false;

			while (at != to) {
				fmt::Obj *next = fmt::objSkip(at);

				if (pinned.has(fmt::toClient(at), next) & !fmt::objIsPad(at)) {
					// Replace previous objects with padding?
					size_t padSize = (byte *)at - (byte *)freeFrom;
					if (padSize > fmt::headerSize) {
						fmt::objMakePad(freeFrom, padSize);
					}

					// Remember where the free space starts.
					freeFrom = next;

					// Check for finalizers.
					finalizers |= fmt::objHasFinalizer(at);
				}

				at = next;
			}

			size_t used = (byte *)freeFrom - (byte *)block->mem(0);
			block->committed(used);
			block->reserved(used);

			if (finalizers)
				block->setFlag(Block::fFinalizers);
			else
				block->clearFlag(Block::fFinalizers);
		}

		struct Finalize {
			void operator ()(void *obj) const {
				if (fmt::hasFinalizer(obj))
					fmt::finalize(obj);
			}
		};

		void Generation::GenChunk::runFinalizers() {
			for (Block *at = (Block *)memory.at; at != (Block *)memory.end(); at = (Block *)at->mem(at->size)) {
				at->traverse(Finalize());
			}
		}

		void Generation::GenChunk::fillSummary(MemorySummary &summary) const {
			for (Block *at = (Block *)memory.at; at != (Block *)memory.end(); at = (Block *)at->mem(at->size)) {
				at->fillSummary(summary);
			}
		}

		void Generation::GenChunk::dbg_verify() {
			size_t free = 0;
			Block *at = (Block *)memory.at;
			while ((byte *)at < (byte *)memory.end()) {
				assert(memory.has(at), L"Invalid block size detected. Leads to outside the specified chunk!");

				if (!at->hasFlag(Block::fUsed))
					free += at->remaining();

				at->dbg_verify();
				at = (Block *)at->mem(at->size);
			}

			assert(at == memory.end(), L"A chunk is overfilled!");
			assert(free == freeBytes, L"The number of free bytes is not correct. "
				L"Stored: " + ::toS(freeBytes) + L", actual: " + ::toS(free));
		}

		void Generation::GenChunk::dbg_dump() {
			PLN(L"Chunk at " << memory << L", " << freeBytes << L" bytes free");

			for (Block *at = (Block *)memory.at; at != (Block *)memory.end(); at = (Block *)at->mem(at->size)) {
				if (at == lastAlloc)
					PNN(L"-> ");
				else
					PNN(L"   ");

				at->dbg_dump();
			}
		}

	}
}

#endif
