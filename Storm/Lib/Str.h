#pragma once
#include "Object.h"
#include "Int.h"

namespace storm {

	STORM_PKG(core);

	/**
	 * The string type used by the generated code.
	 */
	STORM class Str : public Object {
	public:
		// The value of this 'str' object.
		String v;

		// Empty ctor
		STORM Str(Type *type);

		// Copy ctor
		STORM Str(Type *type, const Str &copy);

		// String length.
		Nat STORM_FN count() const;
	};

}
