#pragma once
#include "Object.h"
#include "Int.h"

namespace storm {

	STORM_PKG(core);

	/**
	 * The string type used by the generated code.
	 */
	class Str : public Object {
	public:
		// The value of this 'str' object.
		String v;

		// String length.
		Nat STORM_FN count() const;
	};


	/**
	 * Create the string type.
	 */
	Type *strType();

}
