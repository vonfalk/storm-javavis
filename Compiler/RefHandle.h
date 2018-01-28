#pragma once
#include "Core/Handle.h"
#include "Code/Reference.h"
#include "Code/MemberRef.h"
#include "Code/Binary.h"

namespace storm {
	STORM_PKG(core.lang);

	class Function;

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

		// Set to string function using a thunk.
		void STORM_FN setToS(Function *fn);

		// Set hash function.
		void STORM_FN setHash(code::Ref ref);

		// Set equality function.
		void STORM_FN setEqual(code::Ref ref);

		// Set less-than function.
		void STORM_FN setLess(code::Ref ref);

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

		// Ref to less-than fn.
		code::MemberRef *lessRef;
	};


	// Generate machine code to adapt a operator << function to be callable from a toS function
	// pointer inside a handle.
	code::Binary *STORM_FN toSThunk(Function *fn);


	// Manual population of functions for some of the handles in the system. Will only fill in
	// function pointers, so fill in the proper references later on.
	void populateHandle(Type *type, Handle *h);

}
