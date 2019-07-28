#pragma once
#include "Utils/StackTrace.h"
#include "Thread.h"

namespace os {

	// Gather a stack trace from all UThreads running on a particular thread.
	vector<StackTrace> stackTraces(const Thread &thread);

	// Gather stack traces from all UThreads of the specified Threads. Attempts to capture the stack
	// traces of all threads simultaneously (consistent as long as no other threads interact with
	// these threads).
	vector<vector<StackTrace>> stackTraces(const vector<Thread> &threads);

}
