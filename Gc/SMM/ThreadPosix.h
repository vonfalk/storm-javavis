#pragma once

#if STORM_GC == STORM_GC_SMM && defined(POSIX)

#include "Gc/Scan.h"
#include <ucontext.h>

namespace storm {
	namespace smm {

		// Per-thread information.
		struct ThreadInfo {
			// Number of references.
			size_t refs;

			// Pointer to the thread-local variable, so that we can clear it.
			ThreadInfo **self;

			// If stopped, pointer to the current context.
			ucontext_t *context;

			// Semaphore we wait on when we're stopped.
			Semaphore onResume;

			// Semaphore notified when the thread is actually stopped.
			Semaphore onStop;

			// Create.
			ThreadInfo() : refs(0), onResume(0), onStop(0) {}
		};


		/**
		 * Platform specific threading code for Posix.
		 */
		class OSThread {
		public:
			// Create a handle for the current thread.
			OSThread();

			// Destroy.
			~OSThread();

			// Is this thread running at the moment?
			inline bool running() {
				return stopCount == 0;
			}

			// Ask the scheduler to stop the thread. The thread is not necessarily stopped until
			// 'ensureStop' is called. This is done in two steps to allow multiple stop requests
			// to be issued concurrently. Assumes 'ensureStop' is called afterwards.
			void requestStop();

			// Make sure the thread is actually stopped. Must be called after 'requestStop'.
			void ensureStop();

			// Start the thread.
			void start();

			// Get a thread identifier.
			inline size_t id() const { return tid; }

			// Get the current thread identifier.
			static inline size_t currentId() { return pthread_self(); }

			// Scan the context of a paused thread. Also return the extend of the current stack (ie. the ESP).
			template <class Scanner>
			typename Scanner::Result scan(typename Scanner::Source &source, void **extent) {
				if (extent) {
					// Saved stack pointer.
					*extent = (void *)info->context->uc_mcontext.gregs[REG_RSP];
				}

				mcontext_t *ctx = &info->context->uc_mcontext;
				size_t count = sizeof(*ctx)/sizeof(void *);
				return Scan<Scanner>::array(source, ctx, count);
			}

		private:
			OSThread(const OSThread &o);
			OSThread &operator =(const OSThread &o);

			// Our thread identifier.
			pthread_t tid;

			// Thread-local info.
			ThreadInfo *info;

			// Stop queries.
			size_t stopQueries;

			// How many times did we successfully stop the thread?
			size_t stopCount;
		};

	}
}

#endif
