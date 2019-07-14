#pragma once

#if STORM_GC == STORM_GC_SMM && defined(WINDOWS)

#include "Gc/Scan.h"

namespace storm {
	namespace smm {

		/**
		 * Platform-specific threading code for Windows.
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
			inline size_t id() const { return threadId; }

			// Get the current thread identifier.
			static inline size_t currentId() { return GetCurrentThreadId(); }

			// Scan the context of a paused thread. Also return the extent of the current stack (ie. the ESP).
			template <class Scanner>
			typename Scanner::Result scan(typename Scanner::Source &source, void **extent) {
				if (extent) {
#if defined(X86)
					*extent = (void *)context.Esp;
#elif defined(X64)
					*extent = (void *)context.Rsp;
#else
#error "Unknown architecture, please implement!"
#endif
				}

				return Scan<Scanner>::array(source, &context, sizeof(context)/sizeof(void *));
			}

		private:
			OSThread(const OSThread &o);
			OSThread &operator =(const OSThread &o);

			// A handle to the thread.
			HANDLE handle;

			// Unique thread identifier.
			DWORD threadId;

			// Thread context of the stopped thread.
			CONTEXT context;

			// Stop queries.
			size_t stopQueries;

			// How many times did we successfully stop the thread?
			size_t stopCount;
		};

	}
}

#endif
