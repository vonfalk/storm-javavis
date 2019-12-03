#pragma once

#if STORM_GC == STORM_GC_SMM

#include "VM.h"
#include "VMAlloc.h"
#include "InlineSet.h"
#include "GenSet.h"
#include "History.h"
#include "Gc/MemorySummary.h"
#include "Utils/Templates.h"

namespace storm {
	namespace smm {

		class Root;
		class Block;
		class Thread;
		class Nonmoving;
		class Generation;
		class LockTicket;
		class ArenaTicket;
		class FinalizerPool;

		/**
		 * An Arena keeps track of all allocations made by the GC, and represents the entire world
		 * the GC lives in. The first Arena instance will set up signal- or exception handlers as
		 * necessary in order to get notified by the operating system about important memory events.
		 *
		 * An arena is always backed by virtual memory provided by the operating system.
		 *
		 * The arena also keeps track of a set of mutator threads that may access the memory
		 * allocated inside the arena. Other threads accessing the memory may cause unintended
		 * results.
		 *
		 * In order to access certain functionality of the arena (such as stack scanning), it is
		 * necessary to acquire a ticket by entering the arena. See ArenaTicket.
		 */
		class Arena {
			// Make sure the ArenaTicket class can access the arena. That is often the only way to access
			// some of the state here!
			friend class LockTicket;
			friend class ArenaTicket;

		public:
			// Create the arena, initially try to reserve (but not commit) 'initialSize' bytes of
			// memory for the arena. Also, create 'generationCount' generations, each with the
			// specified size (in bytes).
			Arena(size_t initialSize, const size_t *generations, size_t generationCount);

			// Destroy.
			~Arena();

			// Finalize the arena prematurely.
			void destroy();

			// Get the nursery generation, where all new objects are allocated.
			Generation &nurseryGen() const { return *generations[0]; }

			// Get the area for static allocations.
			Nonmoving &nonmoving() const { return *nonmovingAllocs; }

			// Allocate a chunk of memory from the VMAlloc instance.
			Chunk allocChunk(size_t size, byte identifier);

			// Allocate a chunk of memory, indicating how much memory we would like, but also the
			// bare minimum required.
			Chunk allocChunk(size_t min, size_t preferred, byte identifier);

			// Free a chunk of memory from the VMAlloc instance.
			void freeChunk(Chunk chunk);

			// Query memory information for a pointer. Returns either the generation number or 0xFF on failure.
			byte memGeneration(void *ptr) const {
				return alloc.safeIdentifier(ptr);
			}

			// Attach the current thread to the arena.
			Thread *attachThread();

			// Detach a thread from the arena.
			void detachThread(Thread *thread);

			// Add/remove exact roots.
			void addExact(Root &root);
			void removeExact(Root &root);

			// Add/remove inexact roots.
			void addInexact(Root &root);
			void removeInexact(Root &root);

			// Perform a full GC (API will most likely change).
			void collect();

			// Begin/end ramp allocations.
			void startRamp();
			void endRamp();

			// Object movement history, to allow implementing location dependencies.
			History history;

			// Provide a memory summary. This traverses all objects, and is fairly expensive.
			MemorySummary summary();

			// Check if an address is managed by this arena. Mostly useful for debugging.
			// TODO: Should we lock the access to the VMAlloc instance?
			inline bool has(void *addr) const { return alloc.has(addr); }

			// Verify the contents of the entire arena.
			void dbg_verify();

			// Output a summary.
			void dbg_dump();

		private:
			// No copying!
			Arena(const Arena &o);
			Arena &operator =(const Arena &o);

			// Top-level arena lock, acquired whenever an ArenaTicket is created.
			util::Lock arenaLock;

			// Count the number of time we tried to enter the arena, so that we can detect recursive
			// entries (which will be bad).
			size_t entries;

			// Count the number of ramp allocation requests.
			size_t rampAttempts;

			// Remember which generations asked to be collected during the ramp mode.
			size_t rampBlockedCollections;

			// The generations in use. Objects are promoted from lower to higher numbered
			// generations. Furthermore, we duplicate the last generation passed as a parameter to
			// the constructor so that we can collect it more conveniently. Assuming two long-lived
			// generations, we can simply collect one into the other and swap their locations.
			vector<Generation *> generations;

			// Nonmoving allocations. This is basically a "generation" that is considered belonging to
			// all generations.
			Nonmoving *nonmovingAllocs;

			// Pool for storing objects that need finalization.
			FinalizerPool *finalizers;

			// Virtual memory allocations.
			VMAlloc alloc;

			// Threads running in this arena.
			InlineSet<Thread> threads;

			// Exact roots.
			InlineSet<Root> exactRoots;

			// Inexact roots. We assume there are not too many of these.
			InlineSet<Root> inexactRoots;

			// Perform a garbage collection.
			void collectI(ArenaTicket &e);

			// The ArenaTicket tells us we need to collect certain generations. Returns a GenSet
			// describing the generations actually collected.
			GenSet collectI(ArenaTicket &e, GenSet collect);

			// Swap the last two generations. Should be called after a GC so that the last
			// generation can be collected the next GC cycle.
			void swapLastGens();

		public:
			/**
			 * Enter the arena by acquiring a ticket.
			 *
			 * Note: These are actually implemented in ArenaTicket to resolve header includes, and
			 * since they logically belong to that class anyway.
			 */
			template <class R, class M>
			typename IfNotVoid<R>::t enter(M &memberOf, R (M::*fn)(ArenaTicket &));
			template <class R, class M>
			typename IfVoid<R>::t enter(M &memberOf, R (M::*fn)(ArenaTicket &));
			template <class R, class M, class P>
			typename IfNotVoid<R>::t enter(M &memberOf, R (M::*fn)(ArenaTicket &, P), P p);
			template <class R, class M, class P>
			typename IfVoid<R>::t enter(M &memberOf, R (M::*fn)(ArenaTicket &, P), P p);
			template <class R, class M, class P, class Q>
			typename IfNotVoid<R>::t enter(M &memberOf, R (M::*fn)(ArenaTicket &, P, Q), P p, Q q);
			template <class R, class M, class P, class Q>
			typename IfVoid<R>::t enter(M &memberOf, R (M::*fn)(ArenaTicket &, P, Q), P p, Q q);

			/**
			 * Lock the arena, acquiring a LockTicket.
			 */
			template <class R, class M>
			R lock(M &memberOf, R (M::*fn)(LockTicket &));
			template <class R, class M, class P>
			R lock(M &memberOf, R (M::*fn)(LockTicket &, P), P p);
			template <class R, class M, class P, class Q>
			R lock(M &memberOf, R (M::*fn)(LockTicket &, P, Q), P p, Q q);
		};

	}
}

#endif
