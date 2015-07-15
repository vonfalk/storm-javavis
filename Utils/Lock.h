#pragma once
#include "Windows.h"

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

	private:
		// Underlying implementation.
		CRITICAL_SECTION cs;
	};
}
