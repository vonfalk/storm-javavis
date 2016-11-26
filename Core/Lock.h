#pragma once
#include "Core/Object.h"
#include "OS/Sync.h"

namespace storm {
	STORM_PKG(core.sync);

	/**
	 * A recursive lock usable from within Storm. If possible, use a plain os::Lock or os::Sema as
	 * it does not require any separate allocations.
	 *
	 * This object slightly breaks the expected semantics of Storm as it does not make sense to copy
	 * a semaphore. Instead, all copies will refer to the same semaphore.
	 *
	 * Note: usually it is not required to use locks within Storm as the threading model takes care
	 * of most cases where locks would be needed.
	 *
	 * TODO: Make into a value?
	 */
	class Lock : public Object {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR Lock();
		STORM_CTOR Lock(const Lock &o);

		// Destroy.
		~Lock();

		// Deep copy.
		void STORM_FN deepCopy(CloneEnv *e);

		// Lock guard for C++.
		class L {
		public:
			L(Lock *o);
			~L();
		private:
			Lock *lock;

			L(const L &o);
			L &operator =(const L &o);
		};

	private:
		struct Data {
			size_t refs;
			size_t owner;
			size_t recursion;
			os::Sema sema;

			Data();
		};

		Data *alloc;

		// Lock and unlock.
		void lock();
		void unlock();
	};

}
