#pragma once

/**
 * This file should be included once from a project. It implements the shared variables needed
 * for this project, but beware that if you use dynamic linking, these variables may not be unique!
 */


#include "OS/Shared.h"
#ifdef WINDOWS
#include <process.h>
#endif

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

	void threadCreated() {}
	void threadTerminated() {}

#endif

}

