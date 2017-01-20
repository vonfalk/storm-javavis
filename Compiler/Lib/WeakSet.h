#pragma once
#include "ValueArray.h"
#include "Type.h"

namespace storm {
	STORM_PKG(core.lang);

	// Create types for unknown implementations.
	Type *createWeakSet(Str *name, ValueArray *params);

	/**
	 * Type for weak sets.
	 */
	class WeakSetType : public Type {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR WeakSetType(Str *name, Type *k);

	protected:
		// Load members.
		virtual Bool STORM_FN loadAll();

	private:
		// Content type.
		Type *k;

		// Helpers for creating instances.
		static void createClass(void *mem);
		static void copyClass(void *mem, WeakSetBase *copy);
	};

	/**
	 * The set iterator type.
	 */
	class WeakSetIterType : public Type {
		STORM_CLASS;
	public:
		// Ctor.
		WeakSetIterType(Type *k);

	protected:
		// Lazy loading.
		virtual Bool STORM_FN loadAll();

	private:
		// Content type.
		Type *k;
	};

}
