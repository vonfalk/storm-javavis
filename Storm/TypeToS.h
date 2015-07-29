#pragma once
#include "CodeGen.h"
#include "Function.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Default ToS implementation. Generates the following format:
	 * {<value>, <value>, ... }
	 * Ignores super-classes completely, but tries to show the members in super-classes.
	 */
	class TypeToS : public Function {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR TypeToS(Type *owner);

	private:
		// Generate code.
		CodeGen *CODECALL generateCode();
	};


}
