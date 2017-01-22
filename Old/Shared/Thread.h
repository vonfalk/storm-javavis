#pragma once
#include "Object.h"
#include "OS/Thread.h"

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
	 *
	 * NOTE: Due to initialization order in Engine, do not assume we have access to the
	 * 'myType' at creation in this class!
	 */
	class Thread : public Object {
		STORM_CLASS;
	public:
		// Create a thread.
		STORM_CTOR Thread();

		// Since we're acting like a value, we provide a copy ctor.
		// TODO? Maybe we should become a threaded object?
		STORM_CTOR Thread(Thread *o);

		// Create a thread and use a specific already existing code::Thread object.
		Thread(os::Thread thread);

		// Lazily create the underlying thread.
		Thread(DeclThread::CreateFn fn);

		// Destroy it. This does not enforce that the thread has actually terminated.
		~Thread();

		// Stop the thread we're representing by setting us to the OS thread. This will cause all
		// code from this point to be executed on the calling thread of this function. Only intended
		// to be used during shutdown.
		void stopThread();

		// Get the thread handle.
		const os::Thread &thread();

	private:
		// Thread handle.
		os::Thread osThread;

		// Create using.
		DeclThread::CreateFn create;
	};

}
