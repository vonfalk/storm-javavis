#pragma once

#if STORM_GC == STORM_GC_SMM

#include "Arena.h"
#include "Block.h"
#include "Scanner.h"
#include "Generation.h"
#include "Utils/Lock.h"

namespace storm {
	namespace smm {

		class Arena;
		class ArenaTicket;

		/**
		 * A temporary storage for objects in need of finalization.
		 *
		 * Objects with finalizers that are deemed unreachable during collection are moved here so
		 * that the finalizers may be called at a later point, when the arena lock is not
		 * held. Objects with finalizers are treated as being reachable in some sense, meaning that
		 * these objects may themselves keep other objects alive (such as vtables and type
		 * descriptions). Such objects that are only kept alive by references from objects that are
		 * to be finalized are also moved here so that they can be reclaimed as soon as all
		 * finalizers have been executed. This approach means that we can guarantee that all
		 * references inside objects with finalizers are present during finalization, but not that
		 * these objects themselves have not yet been finalized (the finalization order is
		 * non-deterministic, the collector does not attempt to execute finalizers in a sensible
		 * order).
		 *
		 * This object works as a queue of objects, similarly to the data structure implemented by
		 * ScanState. However, this implementation supports removing (i.e. finalizing) elements
		 * without holding the arena lock, while adding elements requires holding the arena
		 * lock. For this reason, care must be taken in the implementation to make sure that two
		 * potentially different threads do not interfere with each other.
		 *
		 * This class acts like a miniature Generation instance, as it will allocate chunks from the
		 * arena to store the finalizer objects. This generation uses memory allocated with the
		 * finalizerIdentifier id.
		 */
		class FinalizerPool {
		public:
			// Create.
			FinalizerPool(Arena &arena);

			// Destroy.
			~FinalizerPool();

			// The arena.
			Arena &arena;

			// Move an object from its current location into the new generation, replacing the old
			// object with a forwarder to the current location.
			void *move(void *obj);

			// Scan all recently copied objects and recursively move the objects into this pool.
			void scanNew(ArenaTicket &ticket, const Generation::State &source);

			// Call finalizers fo all objects in this pool, and empty the pool afterwards. This
			// operation assumes the global arena lock is *not* held as it executes client code.
			void finalize();

			// Scan all objects currently in the finalizer pool.
			template <class Scanner>
			typename Scanner::Result scan(ArenaTicket &ticket, typename Scanner::Source &source);

			// Add information to a scan summary.
			void fillSummary(MemorySummary &summary) const;

			// Scanner that moves objects into the finalizer pool.
			class Move;

			// Function object that moves objects with finalizers into the finalizer pool. Used with 'traverse'.
			class MoveFinalizers;

		private:
			// No copy!
			FinalizerPool(const FinalizerPool &o);
			FinalizerPool &operator =(const FinalizerPool &o);

			// Lock taken by the thread executing finalizers, making sure that only one thread may
			// run finalizers at any given time, in turn making sure that the 'executing' variable
			// can be used to reach all memory held by this thread.
			util::Lock finalizerLock;

			// Sequence of blocks that contain objects ready for finalization. This variable is
			// modified both while holding the arena lock and without holding any lock, so care must
			// be taken!
			Block *finalizeHead;

			// Objects currently being finalized by a thread.
			Block *executing;

			// The queue used while scanning finalized objects. This queue of blocks is only
			// manipulated while holding the arena lock, and is cleared after 'scanNew' is finished.
			// 'scanFirst' always points to the first block in a chain, making sure that all memory
			// is reachable at all times, so the entire chunk can be committed to 'finalizeHead'
			// when it is all done.
			Block *scanFirst;
			Block *scanHead;
			Block *scanTail;

			// Allocate a new block.
			void newBlock(size_t minSize);

			// Perform a step of scanning. Return 'true' if there might be more to do.
			bool scanStep(const Generation::State &gen);

			// Finalize a chain of blocks. Deallocates the blocks after running finalizers.
			void finalizeChain(Block *first);

			// Deallocate an entire chain.
			void freeChain(Block *first);

			// Finalize all objects in a single block.
			void finalizeBlock(Block *block);

			// Scan a chain of blocks.
			template <class Scanner>
			typename Scanner::Result scanChain(Block *head, typename Scanner::Source &source);

			// Add information to a scan summary.
			void fillSummary(MemorySummary &summary, Block *chain) const;
		};


		/**
		 * Scanner class that moves objects into a FinalizerPool.
		 *
		 * In order to avoid re-scanning objects we've already touched, we make sure to update all
		 * scanned references immediately.
		 */
		class FinalizerPool::Move {
		public:
			struct Params {
				FinalizerPool &to;
				const Generation::State &source;

				Params(FinalizerPool &to, const Generation::State &source)
					: to(to), source(source) {}
			};

			typedef int Result;
			typedef const Params Source;

			FinalizerPool &target;
			Arena &arena;
			byte srcGen;
			const Generation::State &source;

			Move(const Params &p)
				: target(p.to), arena(p.to.arena), srcGen(p.source.identifier()), source(p.source) {}

			inline ScanOption object(void *, void *) const {
				return scanAll;
			}

			inline bool fix1(void *ptr) {
				return arena.memGeneration(ptr) == srcGen;
			}

			inline Result fix2(void **ptr) {
				void *obj = *ptr;
				void *end = (char *)fmt::skip(obj) - fmt::headerSize;

				// Don't touch pinned objects!
				if (source.isPinned(obj, end))
					return 0;

				// We don't need to copy forwarders. Note: Will update the pointer if needed!
				if (fmt::isFwd(obj, ptr))
					return 0;

				// TODO: Error handling!
				*ptr = target.move(obj);
				return 0;
			}

			SCAN_FIX_HEADER
		};


		class FinalizerPool::MoveFinalizers {
		public:
			FinalizerPool &to;
			PinnedSet pinned;

			MoveFinalizers(FinalizerPool &to, const PinnedSet &pinned) : to(to), pinned(pinned) {}

			void operator ()(void *obj) const {
				if (fmt::hasFinalizer(obj)) {
					// Don't touch pinned objects!
					void *end = (char *)fmt::skip(obj) - fmt::headerSize;
					if (!pinned.has(obj, end)) {
						to.move(obj);
					}
				}
			}
		};


		// Scanning.
		template <class Scanner>
		typename Scanner::Result FinalizerPool::scan(ArenaTicket &, typename Scanner::Source &source) {
			typename Scanner::Result r;
			r = scanChain<Scanner>(scanFirst, source);
			if (r != typename Scanner::Result())
				return r;
			r = scanChain<Scanner>(finalizeHead, source);
			if (r != typename Scanner::Result())
				return r;
			return scanChain<Scanner>(executing, source);
		}

		template <class Scanner>
		typename Scanner::Result FinalizerPool::scanChain(Block *head, typename Scanner::Source &source) {
			for (Block *at = head; at; at = at->next()) {
				typename Scanner::Result r = at->scan<Scanner>(source);
				if (r != typename Scanner::Result())
					return r;
			}
			return typename Scanner::Result();
		}

	}
}

#endif
