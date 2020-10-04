#pragma once
#include "Core/Array.h"
#include "Core/Map.h"
#include "Compiler/Syntax/Rule.h"
#include "Compiler/Syntax/Production.h"
#include "Compiler/Syntax/ParentReq.h"

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

				// Allocated parent requirement ID. Empty if no ID allocated.
				ParentReq requirement;

				// Add a production. Assumed to be unique.
				void add(Production *production);

				// Same syntax?
				Bool sameSyntax(RuleInfo *other);
			};

		}
	}
}
