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

#ifdef WINDOWS
		// Thread handle for the running thread.
		HANDLE thread;

		friend DWORD WINAPI ioMain(void *param);
#endif
	};

}
