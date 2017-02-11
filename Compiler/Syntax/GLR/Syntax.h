#pragma once
#include "Core/Array.h"
#include "Core/Map.h"
#include "Core/EnginePtr.h"
#include "Compiler/Syntax/Rule.h"
#include "Compiler/Syntax/Production.h"

namespace storm {
	namespace syntax {
		namespace glr {
			STORM_PKG(lang.bnf.glr);

			/**
			 * Information about a rule.
			 */
			class RuleInfo {
				STORM_VALUE;
			public:
				// Create.
				STORM_CTOR RuleInfo();

				// Count.
				inline Nat STORM_FN count() const { return productions ? productions->count() : 0; }

				// Access elements.
				inline Nat STORM_FN operator[](Nat id) const { return productions ? productions->at(id) : 0; }

				// Add a production.
				void push(Engine &e, Nat id);

			private:
				// All productions for this rule. May be null.
				Array<Nat> *productions;
			};

			// Add a production.
			void STORM_FN push(EnginePtr e, RuleInfo &to, Nat id);

			/**
			 * All syntax in a parser.
			 *
			 * Assigns an identifier to each production to make things easier down the line.
			 */
			class Syntax : public ObjectOn<Compiler> {
				STORM_CLASS;
			public:
				// Create.
				STORM_CTOR Syntax();

				// All known rules.
				Map<Rule *, RuleInfo> *rules;

				// All known productions and their ID:s.
				Map<Production *, Nat> *lookup;

				// All productions. A production's id can be found in 'lookup'.
				Array<Production *> *productions;

				// Add syntax.
				void STORM_FN add(Rule *rule);
				void STORM_FN add(Production *type);

				// Same syntax as another object?
				Bool STORM_FN sameSyntax(Syntax *o);
			};

		}
	}
}
