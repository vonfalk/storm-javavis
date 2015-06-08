#include "stdafx.h"
#include "OS/Shared.h"
#include <process.h>

namespace os {

	/**
	 * Implementation of the missing functions from 'OS' lib.
	 */


	static THREAD ThreadData *currThreadData = null;

	ThreadData *currentThreadData() {
		return currThreadData;
	}

	void currentThreadData(ThreadData *data) {
		currThreadData = data;
	}

	static THREAD UThreadState *currUThreadState = null;

	UThreadState *currentUThreadState() {
		return currUThreadState;
	}

	void currentUThreadState(UThreadState *state) {
		currUThreadState = state;
	}

#ifdef DEBUG

	static nat aliveThreads = 0;

	static void checkThreads() {
		assert(aliveThreads == 0, L"Some threads have not terminated before process exit!");
	}

	void threadCreated() {
		static bool first = true;
		if (first) {
			atexit(&checkThreads);
			first = false;
		}
		aliveThreads++;
	}

	void threadTerminated() {
		aliveThreads--;
	}

#else

	static void threadCreated() {}
	static void threadTerminated() {}

#endif

}

