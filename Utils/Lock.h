#pragma once
#include "Win32.h"

namespace util {
	/**
	 * A simple lock. Allows recursive entry.
	 */
	class Lock : NoCopy {
	public:
		// Create.
		Lock();

		// Destroy.
		~Lock();

		// Lock the lock, and make sure it is automatically unlocked at the end of the scope.
		class L {
		public:
			L(Lock &l);
			L(Lock *l);
			~L();
		private:
			L(const L &);
			L &operator =(const L &);

			Lock *l;
		};

		// Manual lock and unlock (required in some cases).
		void lock();
		void unlock();

	private:
		Lock(const Lock &o);
		Lock &operator =(const Lock &o);

		// Underlying implementation.
#if defined(WINDOWS)
		CRITICAL_SECTION cs;
#elif defined(POSIX)
		pthread_mutex_t cs;
#endif
	};
}
