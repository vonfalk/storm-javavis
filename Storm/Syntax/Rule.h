#pragma once
#include "Type.h"
#include "Parse.h"

namespace storm {
	namespace syntax {

		/**
		 * A syntax rule.
		 *
		 * A rule is represented in the type system as a type. All options specifying this rule
		 * inherit from this type, providing access to what was matched. The base class provides a
		 * member 'eval' for transforming the syntax tree as specified in the syntax file.
		 */
		class Rule : public Type {
			STORM_CLASS;
		public:
			// Create the rule.
			STORM_CTOR Rule(Par<RuleDecl> rule, Scope scope);

			// Declared at.
			STORM_VAR SrcPos pos;

			// Scope.
			Scope scope;

			// More to come...

			// Lazy-loading.
			virtual Bool STORM_FN loadAll();
		};

	}
}
