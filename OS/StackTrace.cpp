#include "stdafx.h"
#include "StackTrace.h"
#include "Utils/Semaphore.h"

namespace os {

	/**
	 * Global trace data. Shared information.
	 */
	struct SharedTrace {
		// Semaphore used to signal from the threads.
		Sema signal;

		// Semaphore used by the threads to wait for event.
		Semaphore wait;

		// The main thread.
		Thread main;

		// Create.
		SharedTrace() : signal(0), wait(0), main(Thread::current()) {}
	};

	/**
	 * An instance of this class is created for each thread we are creating stack traces for.
	 * This class keeps track of the state in each thread, and contains functions executed
	 * at various stages of the capture.
	 */
	struct TraceData {
		// Shared data.
		SharedTrace &shared;

		// Traces captured from the UThreads.
		vector<StackTrace> results;

		// Create.
		TraceData(SharedTrace &shared) : shared(shared) {}

		// Main function for the trace.
		void main() {
			prepare();

			// Wait for our signal!
			shared.wait.down();

			capture(false);
		}

		// Preparations.
		void prepare() {
			// Make it known that we're alive.
			shared.signal.up();
		}

		// Capture.
		void capture(bool thisThread) {
			if (thisThread)
				results.push_back(::stackTrace(2));

			UThreadState *current = UThreadState::current();
			vector<UThread> stacks = current->idleThreads();

			results.reserve(stacks.size());
			for (size_t i = 0; i < stacks.size(); i++) {
				if (!stacks[i].detour(util::memberVoidFn(this, &TraceData::captureUThread))) {
					WARNING(L"Failed to execute detour!");
				}
			}

			// Signal again.
			shared.signal.up();
		}

		// Function called from all UThreads in the context of that UThread. Performs the actual
		// stack trace capturing.
		void captureUThread() {
			results.push_back(::stackTrace(2));
		}
	};

	vector<vector<StackTrace>> stackTraces(const vector<Thread> &threads) {
		SharedTrace shared;
		vector<TraceData> data(threads.size(), TraceData(shared));

		size_t thisThread = threads.size();

		// Inject a UThread into each thread we're interested in.
		for (size_t i = 0; i < threads.size(); i++) {
			if (threads[i] == shared.main) {
				data[i].prepare();
				thisThread = i;
			} else {
				UThread::spawn(util::memberVoidFn(&data[i], &TraceData::main), &threads[i]);
			}
		}

		// Wait for them all to become ready.
		for (size_t i = 0; i < threads.size(); i++) {
			shared.signal.down();
		}

		// Start the capture!
		for (size_t i = 0; i < threads.size(); i++) {
			shared.wait.up();
		}

		// If we're one of the threads, we also need to do some work...
		if (thisThread < threads.size()) {
			data[thisThread].capture(true);
		}

		// Wait for all to complete.
		for (size_t i = 0; i < threads.size(); i++) {
			shared.signal.down();
		}

		// Collect the results.
		vector<vector<StackTrace>> result(threads.size(), vector<StackTrace>());
		for (size_t i = 0; i < threads.size(); i++) {
			result[i] = data[i].results;
		}
		return result;
	}

	vector<StackTrace> stackTraces(const Thread &thread) {
		vector<Thread> threads(1, thread);
		return stackTraces(threads)[0];
	}

}
