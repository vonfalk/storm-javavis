#pragma once

#if STORM_GC == STORM_GC_SMM

#include "VM.h"
#include "Generation.h"
#include "Thread.h"

namespace storm {
	namespace smm {

		class Block;

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

			// Allocate a block with a specified minimum size. The actual size may be larger. 'size'
			// excludes the bytes required for the Block-header.
			Block *allocMin(size_t size);

			// Free a block.
			void free(Block *block);

			// Attach the current thread to the arena.
			Thread *attachThread();

			// Detach a thread from the arena.
			void detachThread(Thread *thread);

			// Perform a full GC (API will most likely change).
			void collect();


			// Scanning.
			template <class Scanner>
			typename Scanner::Result scanRoots(typename Scanner::Source &source) {
				typename Scanner::Result r;
				for (InlineSet<Thread>::iterator i = threads.begin(); i != threads.end(); ++i) {
					r = i->scan<Scanner>(source);
					if (r != typename Scanner::Result())
						return r;
				}
				return r;
			}

		private:
			// No copying!
			Arena(const Arena &o);
			Arena &operator =(const Arena &o);

			// Top-level arena lock. We assume this lock will be taken whenever a thread does
			// something that can impact garbage collection, ie. that could cause issues if a thread
			// would be stopped while owning the lock.
			util::Lock lock;

			// VM management.
			VM *vm;

			// Threads running in this arena.
			InlineSet<Thread> threads;
		};

	}
}

#endif
