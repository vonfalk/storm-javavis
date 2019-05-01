#pragma once

#if STORM_GC == STORM_GC_SMM

#include "VM.h"
#include "VMAlloc.h"
#include "Thread.h"
#include "Gc/MemorySummary.h"

namespace storm {
	namespace smm {

		class Block;
		class Generation;

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
		 */
		class Arena {
		public:
			// Create the arena, initially try to reserve (but not commit) 'initialSize' bytes of
			// memory for the arena. Also, create 'generationCount' generations, each with the
			// specified size (in bytes).
			Arena(size_t initialSize, const size_t *generations, size_t generationCount);

			// Destroy.
			~Arena();

			// Get the nursery generation, where all new objects are allocated.
			Generation &nurseryGen() const { return *generations[0]; }

			// Allocate a chunk of memory from the VMAlloc instance.
			Chunk allocChunk(size_t size, byte identifier);

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

			// Perform a full GC (API will most likely change).
			void collect();

			// Get the buffer. Always of size 'arenaBuffer'.
			inline void **buffer() { return genericBuffer; }

			// Provide a memory summary. This traverses all objects, and is fairly expensive.
			MemorySummary summary();

			// Verify the contents of the entire arena.
			void dbg_verify();

			// Output a summary.
			void dbg_dump();

		private:
			// No copying!
			Arena(const Arena &o);
			Arena &operator =(const Arena &o);

			friend class ArenaEntry;

			// Top-level arena lock. We assume this lock will be taken whenever a thread does
			// something that can impact garbage collection, ie. that could cause issues if a thread
			// would be stopped while owning the lock.
			util::Lock lock;

			// The generations in use. Objects are promoted from lower to higher numbered
			// generations. Furthermore, we duplicate the last generation passed as a parameter to
			// the constructor so that we can collect it more conveniently. Assuming two long-lived
			// generations, we can simply collect one into the other and swap their locations.
			vector<Generation *> generations;

			// Virtual memory allocations.
			VMAlloc alloc;

			// Threads running in this arena.
			InlineSet<Thread> threads;

			// Staticly allocated buffer used by various parts of the system, so we don't have to
			// allocate it on the stack and risk getting a stack overflow at unsuitable times.
			void *genericBuffer[arenaBufferWords];

			// Swap the last two generations. Should be called after a GC so that the last
			// generation can be collected the next GC cycle.
			void swapLastGens();
		};


	}
}

#endif
