#pragma once
#include "ValueArray.h"
#include "Type.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Implements the template interface for the Set<> class in Storm.
	 */

	// Create types for unknown implementations.
	Type *createSet(Str *name, ValueArray *params);

	/**
	 * Type for sets.
	 */
	class SetType : public Type {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR SetType(Str *name, Type *k);

	private:
		// Content type.
		Type *k;
	};

}
