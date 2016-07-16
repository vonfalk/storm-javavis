#pragma once
#include "Object.h"

namespace storm {
	STORM_PKG(core);

	/**
	 * A thread known by Storm.
	 */
	class Thread : public Object {
		STORM_CLASS;
	public:
		// Create a thread.
		STORM_CTOR Thread();

		// Create from a previously started os::Thread object.
		STORM_CTOR Thread(const os::Thread &thread);

		// Since we're not acting as an actor, we need to provide a copy ctor.
		STORM_CTOR Thread(Thread *o);

		// Destroy this thread.
		~Thread();

		// Deep copy as well.
		virtual void STORM_FN deepCopy(CloneEnv *env);

		// Get the thread handle.
		const os::Thread &thread();

	private:
		// Thread handle.
		UNKNOWN(PTR_NOGC) os::Thread osThread;

		// Create using.
		UNKNOWN(PTR_NOGC) DeclThread::CreateFn create;
	};


	/**
	 * Declare the compiler thread, where the compiler itself is to run.
	 */
	STORM_THREAD(Compiler);

}
