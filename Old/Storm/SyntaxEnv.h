#pragma once
#include "Shared/TObject.h"
#include "Thread.h"
#include "Scope.h"

namespace storm {

	/**
	 * Environment object for code generation in the syntax language. Convenient wrapper for the
	 * Scope value, and maybe other nice things in the future.
	 */
	class SyntaxEnv : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR SyntaxEnv(Scope scope);

		// The scope.
		STORM_VAR Scope scope;
	};

}
