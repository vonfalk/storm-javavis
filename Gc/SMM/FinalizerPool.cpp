#include "stdafx.h"
#include "FinalizerPool.h"

#if STORM_GC == STORM_GC_SMM

#include "Arena.h"
#include "Nonmoving.h"

namespace storm {
	namespace smm {

		FinalizerPool::FinalizerPool(Arena &arena)
			: arena(arena), finalizeHead(null), executing(null),
			  scanFirst(null), scanHead(null), scanTail(null) {}

		FinalizerPool::~FinalizerPool() {
			finalizeChain(scanFirst);
			finalizeChain(finalizeHead);
		}

		void *FinalizerPool::move(void *client) {
			fmt::Obj *obj = fmt::fromClient(client);
			size_t size = fmt::objSize(obj);

			if (!scanTail || scanTail->size - scanTail->reserved() < size)
				newBlock(size);

			size_t off = scanTail->reserved();
			fmt::Obj *target = (fmt::Obj *)scanTail->mem(off);
			memcpy(target, obj, size);
			scanTail->reserved(off + size);

			void *clientTarget = fmt::toClient(target);
			fmt::objMakeFwd(obj, size, clientTarget);
			return clientTarget;
		}

		void FinalizerPool::newBlock(size_t minSize) {
			Chunk chunk = arena.allocChunk(minSize, finalizerIdentifier);
			if (chunk.empty()) {
				PLN(L"Failed allocation of " << minSize << L" bytes during GC.");
				dbg_assert(false, L"Out of memory!");
			}
			Block *block = new (chunk.at) Block(chunk.size - sizeof(Block));

			if (scanTail) {
				scanTail->padReserved();
				scanTail->next(block);
			} else {
				// Empty list!
				scanFirst = scanHead = block;
			}

			scanTail = block;
		}

		void FinalizerPool::scanNew(ArenaTicket &, const Generation::State &gen) {
			if (!scanFirst)
				return;

			while (scanStep(gen))
				;

			// At this point, we have scanned all objects, so we can publish the chunk of objects we
			// keep to the world for consumption.

			// Make sure we can use the 'next' pointer in the last block if we will need it.
			scanTail->padReserved();

			// Publish the entire chain of blocks we produced. Since 'finalizeHead' is shared, we
			// need to take care!
			// Note that the ABA problem can not occur here, since we know that we're the only
			// producer of content, since we're holding the global arena lock.
			Block *old;
			do {
				old = atomicRead(finalizeHead);
				scanTail->next(old);
			} while (atomicCAS(finalizeHead, old, scanFirst) != old);

			// Clear the data, so that we can keep going at a later time!
			scanFirst = scanHead = scanTail = null;
		}

		bool FinalizerPool::scanStep(const Generation::State &gen) {
			Block *b = scanHead;
			if (!b)
				return false;

			size_t from = b->committed();
			size_t to = b->reserved();
			if (from >= to)
				return false;

			typedef ScanNonmoving<Move, fmt::ScanAll, true> Scanner;
			fmt::Scan<Scanner>::objects(Move::Params(*this, gen),
										b->mem(from + fmt::headerSize),
										b->mem(to + fmt::headerSize));
			b->committed(to);

			// Done scanning?
			if (to == b->size) {
				Block *next = b->next();
				if (next) {
					scanHead = next;
				} else {
					// Nothing more to do now...
					return false;
				}
			}

			return true;
		}

		void FinalizerPool::finalize() {
			// Early out, so we don't always have to take the lock.
			Block *finalize = atomicRead(finalizeHead);
			if (!finalize)
				return;

			// Something is probably already running, we don't have to try.
			if (atomicRead(executing))
				return;

			{
				util::Lock::L z(finalizerLock);

				// Re-check that we're alone.
				if (atomicRead(executing))
					return;

				// Grab a chunk of memory. Note: We need to do this atomically wrt any scanning threads.
				do {
					finalize = atomicRead(finalizeHead);
					// Nothing to do anymore...
					if (!finalize) {
						atomicWrite(executing, (Block *)null);
						return;
					}

					// Set 'executing' now, so that all objects are reachable at all times if the
					// CAS below succeeds. If it fails, 'executing' will still refer to valid
					// memory, but some objects may be scanned twice for a brief period of time.
					atomicWrite(executing, finalize);
				} while (atomicCAS(finalizeHead, finalize, null) != finalize);

				// At this point, we know we're alone in executing finalizers!
				for (Block *at = finalize; at; at = at->next())
					finalizeBlock(at);

				// Now, we don't need objects to be valid anymore!
				atomicWrite(executing, (Block *)null);

				freeChain(finalize);
			}
		}

		void FinalizerPool::finalizeChain(Block *chain) {
			for (Block *at = chain; at; at = at->next())
				finalizeBlock(at);

			// Everything is finalized. Now, we can deallocate it all!
			freeChain(chain);
		}

		void FinalizerPool::freeChain(Block *first) {
			while (first) {
				Block *next = first->next();
				arena.freeChunk(Chunk(first, first->size + sizeof(Block)));
				first = next;
			}
		}

		void FinalizerPool::finalizeBlock(Block *block) {
			fmt::Obj *at = (fmt::Obj *)block->mem(0);
			fmt::Obj *to = (fmt::Obj *)block->mem(block->committed());

			for (; at != to; at = fmt::objSkip(at)) {
				if (fmt::objHasFinalizer(at))
					fmt::objFinalize(at);
			}
		}

		void FinalizerPool::fillSummary(MemorySummary &summary) const {
			// Note: This isn't entirely safe to do actually...
			fillSummary(summary, atomicRead(finalizeHead));
			fillSummary(summary, scanFirst);
		}

		void FinalizerPool::fillSummary(MemorySummary &summary, Block *chain) const {
			for (Block *at = chain; at; at = at->next())
				at->fillSummary(summary);
		}

	}
}

#endif
