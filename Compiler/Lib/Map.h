#pragma once
#include "ValueArray.h"
#include "Type.h"

namespace storm {
	STORM_PKG(core.lang);

	class MapBase;

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

		// Late initialization.
		virtual void lateInit();

	protected:
		// Load members.
		virtual Bool STORM_FN loadAll();

	private:
		// Content types.
		Type *k;
		Type *v;

		// Add the 'at' member if applicable.
		void addAccess();

		// Helpers for creating instances.
		static void createClass(void *mem);
		static void copyClass(void *mem, MapBase *copy);
	};

	/**
	 * The map iterator type.
	 */
	class MapIterType : public Type {
		STORM_CLASS;
	public:
		// Ctor.
		MapIterType(Type *k, Type *v);

	protected:
		// Lazy loading.
		virtual Bool STORM_FN loadAll();

	private:
		// Content type.
		Type *k;
		Type *v;
	};

}
