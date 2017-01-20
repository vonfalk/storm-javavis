#pragma once
#include "ValueArray.h"
#include "Type.h"

namespace storm {
	STORM_PKG(core.lang);

	class SetBase;

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

	protected:
		// Load members.
		virtual Bool STORM_FN loadAll();

	private:
		// Content type.
		Type *k;

		// Helpers for creating instances.
		static void createClass(void *mem);
		static void copyClass(void *mem, SetBase *copy);
	};

	/**
	 * The set iterator type.
	 */
	class SetIterType : public Type {
		STORM_CLASS;
	public:
		// Ctor.
		SetIterType(Type *k);

	protected:
		// Lazy loading.
		virtual Bool STORM_FN loadAll();

	private:
		// Content type.
		Type *k;
	};

}
