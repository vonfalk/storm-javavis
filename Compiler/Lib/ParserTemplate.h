#pragma once
#include "Type.h"
#include "ValueArray.h"
#include "Compiler/Syntax/Rule.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Implements the template interface for Parser<> in Storm.
	 */

	// Create types for unknown implementations.
	Type *createParser(Str *name, ValueArray *params);

	/**
	 * Type for Parsers.
	 */
	class ParserType : public Type {
		STORM_CLASS;
	public:
		// Create.
		ParserType(Str *name, syntax::Rule *root);

		// Root syntax rule.
		syntax::Rule *root;

	protected:
		// Load members.
		virtual Bool STORM_FN loadAll();
	};

	// Get a parser type for a specific rule.
	ParserType *parserType(syntax::Rule *rule);

}
