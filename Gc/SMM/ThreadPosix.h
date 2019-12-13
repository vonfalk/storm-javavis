#pragma once

#if STORM_GC == STORM_GC_SMM && defined(POSIX)

namespace storm {
	namespace smm {

		class ThreadInfo;

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

			// Thread-local info.
			ThreadInfo *info;
		};

	}
}

#endif
