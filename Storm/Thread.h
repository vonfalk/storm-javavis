#pragma once
#include "Lib/Object.h"
#include "Code/Thread.h"

namespace storm {

	STORM_PKG(core);

	/**
	 * Defines a thread for use in the compiler. If a named thread is needed in the compiler,
	 * declare it like this: STORM_THREAD(<name>). This will create a variable with name
	 * <name> from which the thread can be accessed with <name>.thread(engine). (returns borrowed ptr).
	 * To declare that a specific class should belong to a specific thread, use <TODO>
	 * This does not enforce calls from C++ to be executed on different threads!
	 *
	 * The thread object itself creates a thread when constructed, and keeps that thread
	 * alive until the object is destroyed. The thread may be implemented in any way,
	 * but the implementation has to ensure that different functions running under the
	 * same thread will never have a race-condition other than ones introduced by calling
	 * UThread::leave().
	 */
	class Thread : public Object {
		STORM_CLASS;
	public:
		// Create a thread.
		STORM_CTOR Thread();

		// Create a thread and use a specific already existing code::Thread object.
		Thread(code::Thread thread);

		// Destroy it. This does not enforce that the thread has actually terminated.
		~Thread();

		// Thread handle.
		const code::Thread thread;

	};

	// This is the thread for the compiler.
	// TODO: Where should this be? Is core good?
	STORM_THREAD(Compiler);
}
