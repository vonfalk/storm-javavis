#pragma once
#include "Object.h"
#include "Int.h"

namespace storm {

	STORM_PKG(core);

	/**
	 * The string type used by the generated code.
	 */
	class Str : public Object {
		STORM_CLASS;
	public:
		// The value of this 'str' object.
		String v;

		// Empty ctor
		STORM_CTOR Str(Type *type);

		// Copy ctor
		STORM_CTOR Str(Type *type, const Str &copy);

		// Create from regular string.
		Str(Engine &e, const String &s);

		// String length.
		Nat STORM_FN count() const;
	};

}
