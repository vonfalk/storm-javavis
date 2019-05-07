#pragma once

#if STORM_GC == STORM_GC_SMM

#include "VM.h"
#include "VMAlloc.h"
#include "InlineSet.h"
#include "Gc/MemorySummary.h"
#include <csetjmp>

namespace storm {
	namespace smm {

		class Thread;
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

			// Provide a memory summary. This traverses all objects, and is fairly expensive.
			MemorySummary summary();

			// Verify the contents of the entire arena.
			void dbg_verify();

			// Output a summary.
			void dbg_dump();

			/**
			 * Operations on the arena that requires holding the arena lock and collecting some
			 * information that allows garbage collection to occur.
			 *
			 * Initialization of the Entry class is a bit special. Therefore, it can not be
			 * constructed as a regular class. Rather, use the 'withEntry' function provided in the
			 * Arena class.
			 *
			 * Furthermore, we assume that Entry instances reside on the stack at the last location
			 * where references to garbage collected memory may be stored by the client
			 * program. This means that when an Entry is held, no client code may be called.
			 */
			class Entry {
			private:
				friend class Arena;

				// Create.
				Entry(Arena &arena);
				Entry();
				Entry(const Entry &o);
				Entry &operator =(const Entry &o);

			public:
				// Destroy.
				~Entry();

				// Collected state of the stack when we entered the arena.
				std::jmp_buf buf;

				// Scan all stacks using the given scanner. This will scan inexact references.
				template <class Scanner>
				typename Scanner::Result scanStackRoots(typename Scanner::Source &source);

				// Scan all blocks in all generations that may refer to a block in the given
				// generation. The given generation is never scanned.
				template <class Scanner>
				typename Scanner::Result scanGenerations(typename Scanner::Source &source, Generation *curr);

			private:
				// Associated arena.
				Arena &owner;
			};

			// Create an ArenaEntry instance and call a specified function.
			template <class R, class M>
			R withEntry(M &memberOf, R (M::*fn)(Entry &)) {
				Entry e(*this);
				setjmp(e.buf);
				return (memberOf.*fn)(e);
			}
			template <class R, class M, class P>
			R withEntry(M &memberOf, R (M::*fn)(Entry &, P), P p) {
				Entry e(*this);
				setjmp(e.buf);
				return (memberOf.*fn)(e, p);
			}
			template <class R, class M, class P, class Q>
			R withEntry(M &memberOf, R (M::*fn)(Entry &, P, Q), P p, Q q) {
				Entry e(*this);
				setjmp(e.buf);
				return (memberOf.*fn)(e, p, q);
			}

		private:
			// No copying!
			Arena(const Arena &o);
			Arena &operator =(const Arena &o);

			// Top-level arena lock, acquired whenever an Entry is created.
			util::Lock lock;

			// Count the number of time we tried to enter the arena, so that we can detect recursive
			// entries (which will be bad).
			size_t entries;

			// The generations in use. Objects are promoted from lower to higher numbered
			// generations. Furthermore, we duplicate the last generation passed as a parameter to
			// the constructor so that we can collect it more conveniently. Assuming two long-lived
			// generations, we can simply collect one into the other and swap their locations.
			vector<Generation *> generations;

			// Virtual memory allocations.
			VMAlloc alloc;

			// Threads running in this arena.
			InlineSet<Thread> threads;

			// Perform a garbage collection.
			void collectI(Entry &e);

			// Swap the last two generations. Should be called after a GC so that the last
			// generation can be collected the next GC cycle.
			void swapLastGens();
		};


		/**
		 * Implementation of template members.
		 */


		template <class Scanner>
		typename Scanner::Result Arena::Entry::scanStackRoots(typename Scanner::Source &source) {
			typename Scanner::Result r;
			InlineSet<Thread> &threads = owner.threads;
			for (InlineSet<Thread>::iterator i = threads.begin(); i != threads.end(); ++i) {
				r = i->scan<Scanner>(source, *this);
				if (r != typename Scanner::Result())
					return r;
			}
			return r;
		}


		template <class Scanner>
		typename Scanner::Result Arena::Entry::scanGenerations(typename Scanner::Source &source, Generation *current) {
			typename Scanner::Result r;
			GenSet scanFor;
			scanFor.add(current->identifier);

			for (size_t i = 0; i < owner.generations.size(); i++) {
				Generation *gen = owner.generations[i];

				// Don't scan the current generation, we want to handle that with more care.
				if (gen == current)
					continue;

				// Scan it, instructing the generation to only scan references to the current generation.
				r = gen->scan<Scanner>(scanFor, source);
				if (r != typename Scanner::Result())
					return r;
			}

			return r;
		}

	}
}

#endif
