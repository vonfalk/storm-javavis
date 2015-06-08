#pragma once

namespace os {

	/**
	 * This file declares an interface that has to be implemented elsewhere. This is needed to make
	 * sure we only have one instance of critical shared variables, even though we dynamically link
	 * shared libraries which may include a static version of this library multiple times.
	 *
	 * These are not designed to be used on their own, take care!
	 */

	// Access the 'current thread' variable. Thread local variable, initialized to null.
	class ThreadData;
	ThreadData *currentThreadData();
	void currentThreadData(ThreadData *data);

	// Access the current UThreadState. Initialized to null.
	class UThreadState;
	UThreadState *currentUThreadState();
	void currentUThreadState(UThreadState *state);

	// During debug: keep track of the number of alive threads.
	void threadCreated();
	void threadTerminated();


	// Struct holding pointers to all functions defined here, for forwarding later.
	struct OsFns {
		typedef ThreadData *(*GetThreadData)();
		GetThreadData getThreadData;

		typedef void (*SetThreadData)(ThreadData *);
		SetThreadData setThreadData;

		typedef UThreadState *(*GetUThreadState)();
		GetUThreadState getUThreadState;

		typedef void (*SetUThreadState)(UThreadState *);
		SetUThreadState setUThreadState;

		typedef void (*ThreadNotify)();
		ThreadNotify threadCreated, threadTerminated;
	};

	// Initialize the struct above.
	inline OsFns osFns() {
		OsFns f = {
			&currentThreadData,
			&currentThreadData,
			&currentUThreadState,
			&currentUThreadState,
			&threadCreated,
			&threadTerminated,
		};
		return f;
	}

}
