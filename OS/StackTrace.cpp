#include "stdafx.h"
#include "StackTrace.h"
#include "Utils/Semaphore.h"

namespace os {

	vector<StackTrace> stackTraces(const Thread &thread) {
		vector<Thread> threads(1, thread);
		return stackTraces(threads)[0];
	}


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
				results.push_back(::stackTrace(1));

			// TODO: Perhaps we should lock our read of this list.
			const InlineSet<UThreadStack> &stacks = Thread::current().stacks();
			InlineSet<UThreadStack>::iterator at = stacks.begin(), end = stacks.end();
			for (; at != end; ++at) {
				if (at->initializing)
					continue;

				// Ignore this thread.
				if (!at->desc)
					continue;

				TODO(L"Capture the uthread!");
				// Here, we want to alter the stack of this thread, so that we may execute code
				// there for a little while before we return it to its original state again.
				results.push_back(StackTrace());
			}

			// Signal again.
			shared.signal.up();
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

}
