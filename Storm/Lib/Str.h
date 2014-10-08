#pragma once
#include "Object.h"

namespace storm {

	/**
	 * The string type used by the generated code.
	 */
	class Str : public Object {
	public:
		// The value of this 'str' object.
		String v;
	};


	/**
	 * Create the string type.
	 */
	Type *strType();
}
