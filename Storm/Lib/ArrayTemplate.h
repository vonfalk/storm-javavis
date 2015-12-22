#pragma once
#include "Shared/Array.h"
#include "Template.h"
#include "Type.h"
#include "Storm/CodeGen.h"

namespace storm {
	STORM_PKG(core.lang);

	// Add the array template class to the package given.
	void addArrayTemplate(Par<Package> to);

	/**
	 * The array type.
	 */
	class ArrayType : public Type {
		STORM_CLASS;
	public:
		// Ctor.
		ArrayType(const Value &param);

		// Parameter type.
		const Value param;

		// Lazy loading.
		virtual bool loadAll();

	private:
		// Load functions assuming param is an object.
		void loadClassFns();

		// Load functions assuming param is a value.
		void loadValueFns();
	};

	/**
	 * The array iterator type.
	 */
	class ArrayIterType : public Type {
		STORM_CLASS;
	public:
		// Ctor.
		ArrayIterType(const Value &param);

		// Parameter type.
		const Value param;

		// Lazy loading.
		virtual bool loadAll();
	};

	// See Array.h for 'arrayType' function.

	// Check if a value represents a Array type.
	Bool STORM_FN isArray(Value v);

	// Get the value of a array type. If 'v' is not a array, 'v' is returned without modification.
	Value STORM_FN unwrapArray(Value v);

	// Wrap a type in array.
	Value STORM_FN wrapArray(Value v);

}
