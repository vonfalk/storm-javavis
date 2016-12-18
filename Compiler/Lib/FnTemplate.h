#pragma once
#include "ValueArray.h"
#include "Type.h"

namespace storm {
	STORM_PKG(core.lang);

	Type *createFn(Str *name, ValueArray *params);

	/**
	 * Type for function pointers.
	 */
	class FnType : public Type {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR FnType(Str *name, ValueArray *params);
	};

	// Find the function type.
	Type *fnType(Array<Value> *params);

}
