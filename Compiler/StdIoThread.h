#pragma once
#include "Core/Io/StdStream.h"
#include "OS/Condition.h"
#include "Utils/Lock.h"

namespace storm {

	/**
	 * A thread dedicated to synchronizing standard IO (at least on Windows, where we can not do
	 * async IO on these handles). This thread is not exposed to the Gc, as it only manages pointers
	 * which are pinned on other (suspended) thread's stacks.
	 */
	class StdIo : NoCopy {
	public:
		// Create - spawns a thread.
		StdIo();

		// Destroy - terminates the thread again.
		~StdIo();

		// Post an IO-request.
		void post(StdRequest *request);

	private:
		// Pointer to the first and last nodes. Protected using the lock.
		StdRequest *first;
		StdRequest *last;

		// Lock protecting 'first' and 'last'.
		util::Lock lock;

		// Condition for synchronization.
		os::Condition cond;

		// Post a message to the thread.
		void postThread(StdRequest *request);

		// Platform-specific initialization and destruction.
		void platformInit();
		void platformDestroy();

		// Thread management.
		void startThread();
		void waitThread();
		bool hasThread();

		// Read-write functionality.
		void doNop(StdRequest *r);
		void doRead(StdRequest *r);
		void doWrite(StdRequest *r);

		// Try to perform a nonblocking read operation. Returns 'true' on success.
		bool tryRead(StdRequest *r);

		// Lock input.
		void lockInput();
		bool tryLockInput();
		void unlockInput();

		// Main function for the IO thread.
		void main();

#if defined(WINDOWS)
		// Thread handle for the running thread.
		HANDLE thread;

		// All handles we need so we do not need to query the OS all the time.
		HANDLE handles[3];

		// Lock for doing input.
		CRITICAL_SECTION inputLock;

		// Startup function.
		friend DWORD WINAPI ioMain(void *param);

#elif defined(POSIX)
		// Thread handle.
		pthread_t thread;
		bool threadStarted;

		// All handles used.
		int handles[3];

		// Lock for doing input.
		pthread_mutex_t inputLock;

		// Startup function.
		friend void *ioMain(void *param);

#else
#error "Please implement STDIO for your platform!"
#endif
	};

}
