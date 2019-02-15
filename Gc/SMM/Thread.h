#pragma once

#if STORM_GC == STORM_GC_SMM

#include "InlineSet.h"
#include "Allocator.h"
#include "AddrSet.h"
#include "Gc/Scan.h"
#include "OS/Thread.h"

#include "ThreadWin.h"
#include "ThreadPosix.h"


namespace storm {
	namespace smm {

		class Arena;
		class ArenaEntry;

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

			// Scan the contents of this thread. If this thread is a different thread than the
			// currently executing thread, the thread is assumed to have been successfully stopped
			// at an earlier point in time.
			template <class Scanner>
			typename Scanner::Result scan(typename Scanner::Source &source, ArenaEntry &entry);

		private:
			// No copying.
			Thread(const Thread &o);
			Thread &operator =(const Thread &o);

			// A reference to all UThreads that may be running on this thread, so that we may access
			// them cheaply during scanning.
			const InlineSet<os::UThreadStack> &stacks;

			// Handle to the current thread, along with additional operations not provided by the
			// OS/ threading library (these abilities do not belong there).
			OSThread thread;
		};


		// Implementation of the 'scan' function.
		template <class Scanner>
		typename Scanner::Result Thread::scan(typename Scanner::Source &source, ArenaEntry &entry) {
			typename Scanner::Result r;

			// The extent of the current stack (ie. its ESP).
			void *extent;

			if (thread.running()) {
				// We assume this is the current thread.

				// We only need to set 'extent' to the 'entry' stored on the stack. It contains the
				// state of the thread as it was when we entered the Arena, so that we don't
				// overscan. Since it is on the stack, we only need to make sure to scan the stack
				// from there to get everything in one go.
				extent = &entry;
			} else {
				// A paused thread!
				r = thread.scan<Scanner>(source, &extent);
				if (r != typename Scanner::Result())
					return r;
			}

			return Scan<Scanner>::stacks(source, stacks, extent, null);
		}

	}
}

#endif
