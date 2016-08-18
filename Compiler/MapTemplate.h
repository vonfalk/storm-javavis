#pragma once
#include "ValueArray.h"
#include "Type.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Implements the template interface for the Map<> class in Storm.
	 */

	// Create types for unknown implementations.
	Type *createMap(Str *name, ValueArray *params);

	/**
	 * Type for maps.
	 */
	class MapType : public Type {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR MapType(Str *name, Type *k, Type *v);

	private:
		// Content types.
		Type *k;
		Type *v;
	};

}
