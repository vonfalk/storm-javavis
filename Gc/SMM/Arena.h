#pragma once

#if STORM_GC == STORM_GC_SMM

#include "VM.h"
#include "VMAlloc.h"
#include "Thread.h"

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

			// The generations in use (array).
			Generation *generations;

			// Number of generations.
			const size_t generationCount;

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

		private:
			// No copying!
			Arena(const Arena &o);
			Arena &operator =(const Arena &o);

			friend class ArenaEntry;

			// Top-level arena lock. We assume this lock will be taken whenever a thread does
			// something that can impact garbage collection, ie. that could cause issues if a thread
			// would be stopped while owning the lock.
			util::Lock lock;

			// Virtual memory allocations.
			VMAlloc alloc;

			// Threads running in this arena.
			InlineSet<Thread> threads;

			// Staticly allocated buffer used by various parts of the system, so we don't have to
			// allocate it on the stack and risk getting a stack overflow at unsuitable times.
			void *genericBuffer[arenaBufferWords];
		};


	}
}

#endif
