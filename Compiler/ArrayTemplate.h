#pragma once
#include "ValueArray.h"
#include "Type.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Implements the template interface for the Array<> class in Storm.
	 */

	// Create types for unknown implementations.
	Type *createArray(Str *name, ValueArray *params);

	/**
	 * Type for arrays.
	 */
	class ArrayType : public Type {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR ArrayType(Str *name, Type *contents);

		// Late init.
		virtual void lateInit();

	private:
		// Content type.
		Type *contents;
	};

}
