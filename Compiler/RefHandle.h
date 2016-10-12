#pragma once
#include "Core/Handle.h"
#include "Code/Reference.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * A handle where all members are updated using references.
	 */
	class RefHandle : public Handle {
		STORM_CLASS;
	public:
		STORM_CTOR RefHandle();


	private:
		// Ref to copy ctor (may be null).

		// Ref to dtor (may be null).

		// Ref to deep copy fn.

		// Ref to toS fn.

		// Ref to hash fn.

		// Ref to equality fn.
	};

	/**
	 * Update pointers inside the ref object (we can not store pointers into object when using MPS).
	 */
	class HandleRef : public code::Reference {
		STORM_CLASS;
	public:
		// TODO!
	};

}
