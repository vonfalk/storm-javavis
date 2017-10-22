#pragma once
#include "Function.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Default destructor for a type.
	 *
	 * See if a destructor is needed by using 'needsDestructor' below.
	 */
	class TypeDefaultDtor : public Function {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR TypeDefaultDtor(Type *owner);

		// Pure?
		virtual Bool STORM_FN pure() const;

	private:
		// Owner.
		Type *owner;

		// Generate code.
		CodeGen *CODECALL generate();
	};

	// Check if a destructor is needed for 'type'.
	Bool STORM_FN needsDestructor(Type *type);

}
