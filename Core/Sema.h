#pragma once
#include "Core/Object.h"

namespace storm {
	STORM_PKG(core.sync);

	/**
	 * A basic semaphore usable from whitin Storm. If possible, use a plain os::Lock or os::Sema as
	 * it does not require any separate allocations.
	 *
	 * This object slightly breaks the expected semantics of Storm as it does not make sense to copy
	 * a semaphore. Instead, all copies will refer to the same semaphore.
	 *
	 * TODO: Make into a value?
	 */
	class Sema : public Object {
		STORM_CLASS;
	public:
		// Initialize the semaphore to 1.
		STORM_CTOR Sema();

		// Initialize the semaphore to 'count'.
		Sema(Nat count);

		// Copy.
		Sema(const Sema &o);

		// Destroy.
		~Sema();

		// Deep copy.
		void STORM_FN deepCopy(CloneEnv *e);

		// Increase or decrease the semaphore.
		void STORM_FN up();
		void STORM_FN down();

	private:
		// Separate allocation for the actual lock. Allocated outside of the Gc since we do not
		// allow barriers or movement of the memory allocated for the lock.
		struct Data {
			size_t refs;
			os::Sema sema;

			Data(Nat count);
		};

		// Allocation. Not GC:d.
		Data *alloc;
	};

}
