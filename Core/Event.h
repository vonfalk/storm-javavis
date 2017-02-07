#pragma once
#include "Core/Object.h"

namespace storm {
	STORM_PKG(core.sync);

	/**
	 * A basic event usable from within Storm. If possible, use a plain os::Event as it does not
	 * require any separate allocations.
	 *
	 * The event is initially cleared, which means that any calls to 'wait' will block until another
	 * thread calls 'set'. The event is then in its signaled state, so any calls to 'wait' will block
	 *
	 * This object slightly breaks the expected semantics of Storm as it does not make sense to copy
	 * a semaphore. Instead, all copies will refer to the same semaphore.
	 *
	 * TODO: Make into a value?
	 */
	class Event : public Object {
		STORM_CLASS;
	public:
		// Initialize.
		STORM_CTOR Event();

		// Copy.
		Event(const Event &o);

		// Destroy.
		~Event();

		// Deep copy.
		void STORM_FN deepCopy(CloneEnv *e);

		// Set/clear the event.
		void STORM_FN set();
		void STORM_FN clear();

		// Wait for the event to get signaled.
		void STORM_FN wait();

		// See if the condition is set.
		Bool STORM_FN isSet();

	private:
		// Separate allocation for the actual lock. Allocated outside of the Gc since we do not
		// allow barriers or movement of the memory allocated for the lock.
		struct Data {
			size_t refs;
			os::Event event;

			Data();
		};

		// Allocation. Not GC:d.
		Data *alloc;
	};

}
