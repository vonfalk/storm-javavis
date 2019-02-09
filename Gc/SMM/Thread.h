#pragma once

#if STORM_GC == STORM_GC_SMM

#include "InlineSet.h"
#include "Allocator.h"
#include "OS/Thread.h"

namespace storm {
	namespace smm {

		class Arena;

		/**
		 * A thread known by the GC.
		 *
		 * Allows pausing threads so that registers and the stack that is currently executing can be
		 * scanned as required.
		 */
		class Thread : public SetMember<Thread> {
		public:
			// Create an instance for the current thread.
			Thread(Arena &owner);

			// Allocator specific for this thread.
			Allocator alloc;

		private:
			// No copying.
			Thread(const Thread &o);
			Thread &operator =(const Thread &o);

			// A reference to all UThreads that may be running on this thread, so that we may access
			// them cheaply during scanning.
			const InlineSet<os::UThreadStack> &stacks;
		};

	}
}

#endif
