#pragma once
#include "NamedThread.h"

namespace storm {

	// Constraints for which threads a specific function or variable may be accessed from.
	// See RunOn::State for the possible states.
	class RunOn {
		STORM_VALUE;
	public:
		// Determines the state.
		enum State {
			// Run anywhere. This is used when no threading constraint has been declared,
			// and is the default value of this class.
			any,

			// The thread to run on is decided runtime. For example when a class has been
			// declared to run on a thread provided runtime.
			runtime,

			// The thread is known compile-time. Find the thread in the 'thread' member.
			named,
		};

		// State.
		State state;

		// Thread. Only valid if 'state == named'.
		Auto<NamedThread> thread;

		// Create.
		RunOn(State s = any);
		RunOn(Par<NamedThread> thread);

		// Assuming we are running on a thread represented by this, may we run a function
		// declared to run on 'other'?
		Bool STORM_FN canRun(const RunOn &other) const;
	};

	// Create some RunOn objects (for Storm).
	RunOn STORM_FN runOn(Par<NamedThread> thread);
	RunOn STORM_FN runOnAny();
	RunOn STORM_FN runOnRuntime();

	// Output.
	wostream &operator <<(wostream &to, const RunOn &v);

}
