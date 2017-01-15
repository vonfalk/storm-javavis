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

		// Parameter.
		Value param() const;

	protected:
		// Lazy loading.
		virtual Bool STORM_FN loadAll();

	private:
		// Content type.
		Type *contents;

		// Helpers.
		void loadClassFns();
		void loadValueFns();
	};

	/**
	 * The array iterator type.
	 */
	class ArrayIterType : public Type {
		STORM_CLASS;
	public:
		// Ctor.
		ArrayIterType(Type *param);

	protected:
		// Lazy loading.
		virtual Bool STORM_FN loadAll();

	private:
		// Content type.
		Type *contents;
	};

	Bool STORM_FN isArray(Value v);
	Value STORM_FN unwrapArray(Value v);
	Value STORM_FN wrapArray(Value v);

}
