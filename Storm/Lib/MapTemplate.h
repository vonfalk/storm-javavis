#pragma once
#include "Shared/Map.h"
#include "Template.h"
#include "Type.h"
#include "Storm/CodeGen.h"

namespace storm {
	STORM_PKG(core);

	// Add the map type to the package given.
	void addMapTemplate(Par<Package> to);

	/**
	 * The map type.
	 */
	class MapType : public Type {
		STORM_CLASS;
	public:
		// Ctor.
		MapType(const Value &key, const Value &value);

		// Key and value type.
		const Value key, value;

		// Lazy loading.
		virtual bool loadAll();
	};

	// See Map.h for 'mapType' function.

}
