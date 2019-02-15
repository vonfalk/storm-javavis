#pragma once

#if STORM_GC == STORM_GC_SMM

#include "Utils/Lock.h"
#include "Thread.h"
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

			// Scanning.
			template <class Scanner>
			typename Scanner::Result scanRoots(typename Scanner::Source &source) {
				typename Scanner::Result r;
				InlineSet<Thread> &threads = this->threads();
				for (InlineSet<Thread>::iterator i = threads.begin(); i != threads.end(); ++i) {
					r = i->scan<Scanner>(source, *this);
					if (r != typename Scanner::Result())
						return r;
				}
				return r;
			}

		private:
			// Associated arena.
			Arena &owner;

			// The lock we're holding inside the arena.
			util::Lock::L lock;

			// Get all threads from the arena (needs to be done in the cpp-file).
			InlineSet<Thread> &threads();
		};

	}
}

#endif
