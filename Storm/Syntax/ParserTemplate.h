#pragma once
#include "Type.h"
#include "Rule.h"

namespace storm {
	namespace syntax {
		STORM_PKG(core.lang);

		// Add the template.
		void addParserTemplate(Engine &to);

		// Get the array type (borrowed ptr).
		Type *parserType(Engine &e, const Value &type);

		/**
		 * Type for the Parser template.
		 */
		class ParserType : public Type {
			STORM_CLASS;
		public:
			// Ctor.
			ParserType(Par<Rule> root);

			// Get the root rule from Storm.
			Rule *STORM_FN root();

			// Root syntax rule (weak ptr).
			Rule *rootRule;

			// Load members.
			virtual Bool STORM_FN loadAll();
		};

	}
}
