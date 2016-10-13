#pragma once
#include "Core/Handle.h"
#include "Code/Reference.h"
#include "Code/MemberRef.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * A handle where all members are updated using references.
	 */
	class RefHandle : public Handle {
		STORM_CLASS;
	public:
		// Create. 'inside' is used to create references.
		STORM_CTOR RefHandle(code::Content *inside);

		// Set copy ctor.
		void STORM_FN setCopyCtor(code::Ref ref);

		// Set destructor.
		void STORM_FN setDestroy(code::Ref ref);

		// Set deep copy function.
		void STORM_FN setDeepCopy(code::Ref ref);

		// Set to string function.
		void STORM_FN setToS(code::Ref ref);

		// Set hash function.
		void STORM_FN setHash(code::Ref ref);

		// Set equality function.
		void STORM_FN setEqual(code::Ref ref);

	private:
		// Content to use when creating references.
		code::Content *content;

		// Ref to copy ctor (may be null).
		code::MemberRef *copyRef;

		// Ref to dtor (may be null).
		code::MemberRef *destroyRef;

		// Ref to deep copy fn.
		code::MemberRef *deepCopyRef;

		// Ref to toS fn.
		code::MemberRef *toSRef;

		// Ref to hash fn.
		code::MemberRef *hashRef;

		// Ref to equality fn.
		code::MemberRef *equalRef;
	};

}
