#pragma once
#include "Core/Object.h"

namespace code {
	STORM_PKG(core.asm);

	class Listing;

	/**
	 * Base class for custom transforms.
	 */
	class Transform : public Object {
		STORM_CLASS;
	public:
		STORM_CTOR Transform();

		// Called before transforming something.
		virtual void STORM_FN before(Listing *dest, Listing *src);

		// Called once for each instruction in 'src'.
		virtual void STORM_FN during(Listing *dest, Listing *src, Nat id);

		// Called after thransforming is done.
		virtual void STORM_FN after(Listing *dest, Listing *src);
	};

	// Transform using a transformer.
	Listing *STORM_FN transform(Listing *src, Transform *use);
}
