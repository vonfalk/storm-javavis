#pragma once
#include "OS/Sync.h"

namespace ssl {

	/**
	 * Generic data utilities.
	 */

	/**
	 * Generic refcount class.
	 */
	class RefCount {
	public:
		// Create, initializes refs to 1.
		RefCount() : refs(1) {}

		// Destroy.
		virtual ~RefCount() {}

		// Add a reference.
		void ref() {
			atomicIncrement(refs);
		}

		// Release a reference.
		void unref() {
			if (atomicDecrement(refs) == 0) {
				delete this;
			}
		}

	private:
		// References.
		size_t refs;
	};

	/**
	 * Data for an SSL context. Internal to the Context, but convenient to have outside the class
	 * declaration.
	 */
	class SSLContext : public RefCount {};

	/**
	 * Data for an individual session.
	 */
	class SSLSession : public RefCount {
	public:
		// Lock for the session.
		os::Lock lock;
	};

}
