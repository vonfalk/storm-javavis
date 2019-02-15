#pragma once

#if STORM_GC == STORM_GC_SMM && defined(POSIX)

namespace storm {
	namespace smm {

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

			// Stop the thread.
			void stop();

			// Start the thread.
			void start();

			// Scan the context of a paused thread. Also return the extend of the current stack (ie. the ESP).
			template <class Scanner>
			typename Scanner::Result scan(typename Scanner::Source &source, void **extent) {
				assert(false, L"TODO");
				return typename Scanner::Result();
			}

		private:
			OSThread(const OSThread &o);
			OSThread &operator =(const OSThread &o);

			// Our thread identifier.
			pthread_t tid;

			// How many times have we tried to stop the thread?
			size_t stopCount;
		};

	}
}

#endif
