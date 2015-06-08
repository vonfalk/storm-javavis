#pragma once
#include "Shared/TObject.h"
#include "Thread.h"

namespace storm {
	STORM_PKG(lang);

	/**
	 * Defines the entry point for a REPL for a language. Any language that
	 * supports a REPL may simply implement a class named "Repl" that inherits
	 * from this class.
	 */
	class LangRepl : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		// Create the REPL object.
		STORM_CTOR LangRepl();

		// Copy.
		STORM_CTOR LangRepl(Par<LangRepl> o);

		// Evaluate a line. Return 'false' if you need another line.
		virtual Bool STORM_FN eval(Par<Str> line);

		// Terminate the REPL?
		virtual Bool STORM_FN exit();
	};

}
