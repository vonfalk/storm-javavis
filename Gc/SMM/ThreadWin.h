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

			// Stop the thread.
			void stop();

			// Start the thread.
			void start();

			// Scan the context of a paused thread. Also return the extent of the current stack (ie. the ESP).
			template <class Scanner>
			typename Scanner::Result scan(typename Scanner::Source &source, void **extent) {
				CONTEXT context;
				if (GetThreadContext(handle, &context)) {
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
				} else {
					dbg_assert(false, L"GetThreadContext failed.");
				}
				return typename Scanner::Result();
			}

		private:
			OSThread(const OSThread &o);
			OSThread &operator =(const OSThread &o);

			// A handle to the thread.
			HANDLE handle;

			// How many times have we tried to stop the thread?
			size_t stopCount;
		};

	}
}

#endif
