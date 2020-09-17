#pragma once
#include "Core/Array.h"
#include "Core/Map.h"
#include "Compiler/Syntax/Rule.h"
#include "Compiler/Syntax/Production.h"

namespace storm {
	namespace syntax {
		namespace ll {
			STORM_PKG(lang.bnf.ll);

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

				// Add a production if it is not already present.
				void add(ProductionType *production);

				// Make sure 'productions' is sorted.
				void sort();

				// Same syntax?
				Bool sameSyntax(RuleInfo *other);
			};

		}
	}
}
