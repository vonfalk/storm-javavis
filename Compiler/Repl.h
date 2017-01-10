#pragma once
#include "Thread.h"

namespace storm {
	STORM_PKG(lang);

	/**
	 * Defines the entry point for a REPL for a language. Any language that supports a REPL may
	 * implement a function 'repl()' which returns an instance of a Repl-class.
	 */
	class Repl : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR Repl();

		// Evaluate a line. Return 'false' if more input is needed.
		virtual Bool STORM_FN eval(Str *line);

		// Terminate the repl?
		virtual Bool STORM_FN exit();
	};

}
