#pragma once
#include "Core/Array.h"
#include "Core/Map.h"
#include "Compiler/Syntax/Rule.h"
#include "Compiler/Syntax/Production.h"

namespace storm {
	namespace syntax {
		namespace gll {
			STORM_PKG(lang.bnf.gll);

			/**
			 * Information about a rule, including a sorted list of its productions.
			 */
			class RuleInfo : public ObjectOn<Compiler> {
				STORM_CLASS;
			public:
				// Create.
				STORM_CTOR RuleInfo(Rule *rule);

				// Rule.
				Rule *rule;

				// Productions. Will eventually be ordered by priority.
				Array<Production *> *productions;

				// Add a production. Assumed to be unique.
				void add(Production *production);

				// Same syntax?
				Bool sameSyntax(RuleInfo *other);
			};

		}
	}
}
