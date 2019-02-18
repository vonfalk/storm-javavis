#pragma once

#if STORM_GC == STORM_GC_SMM

#include "Utils/Lock.h"
#include "Thread.h"
#include "Arena.h"
#include <csetjmp>

namespace storm {
	namespace smm {

		class Arena;

		/**
		 * Class instantiated at entry to the arena. Collects the current thread's state at the
		 * entry so that we may use that state if we need to scan thread roots.
		 *
		 * It is acceptable to assume that this instance is allocated on the stack of the calling thread.
		 */
		class ArenaEntry {
			friend class Arena;

			// Create, only from within Arena. 'buf' must be initialized separately, as 'setjmp'
			// must be called in the surrounding scope.
			ArenaEntry(Arena &owner);

			// No copying.
			ArenaEntry(ArenaEntry &o);
			ArenaEntry &operator =(ArenaEntry &o);
		public:
			// Collected state of the stack when we entered the arena.
			std::jmp_buf buf;

			// Scan all stacks using the given scanner. This will scan inexact references.
			template <class Scanner>
			typename Scanner::Result scanStackRoots(typename Scanner::Source &source);

			// Scan all blocks in all generation that may refer to a block in the current
			// generation. The current generation is never scanned.
			template <class Scanner>
			typename Scanner::Result scanGenerations(typename Scanner::Source &source, Generation *gen, Block *block);

		private:
			// Associated arena.
			Arena &owner;

			// The lock we're holding inside the arena.
			util::Lock::L lock;
		};


		/**
		 * Implementation of template members.
		 */


		template <class Scanner>
		typename Scanner::Result ArenaEntry::scanStackRoots(typename Scanner::Source &source) {
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
		typename Scanner::Result ArenaEntry::scanGenerations(typename Scanner::Source &source, Generation *current, Block *block) {
			typename Scanner::Result r;

			for (size_t i = 0; i < owner.generationCount; i++) {
				Generation *gen = &owner.generations[i];

				// Don't scan the current generation, we want to handle that with more care.
				if (gen == current)
					continue;

				// Skip the entire block if it reports it may not contain any pointers we're interested in.
				if (!gen->mayReferTo(block))
					continue;

				// Go ahead and scan it.
				r = gen->scan<Scanner>(source);
				if (r != typename Scanner::Result())
					return r;
			}

			return r;
		}

	}
}

#endif
