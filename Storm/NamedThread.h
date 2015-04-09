#pragma once
#include "SyntaxObject.h"
#include "Named.h"
#include "Thread.h"

namespace storm {
	STORM_PKG(core);

	/**
	 * Declares a named thread in a package.
	 */
	class NamedThread : public Named {
		STORM_CLASS;
	public:
		// Create.
		NamedThread(const String &name);
		NamedThread(const String &name, Par<Thread> t);
		STORM_CTOR NamedThread(Par<Str> name);
		STORM_CTOR NamedThread(Par<SStr> name);

		// Destroy.
		~NamedThread();

		// Declared at.
		SrcPos pos;

		// Reference to this thread.
		code::Ref ref();

		// Get the thread. (borrowed ptr).
		Thread *thread() const { return myThread.borrow(); }

	protected:
		// Output.
		virtual void output(wostream &to) const;

	private:
		// The thread we are representing. Make sure this stays the same over time, otherwise
		// the thread safety is violated. Also double-check this when considering code updates!
		Auto<Thread> myThread;

		// Reference.
		code::RefSource *reference;
	};


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
