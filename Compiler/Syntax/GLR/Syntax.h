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

			class StackItem;

			/**
			 * Representation of all syntax in a parser.
			 *
			 * To properly handle the extensions supported by the bnf language, we introduce
			 * additional nonterminals to the grammar, and transform them as follows:
			 * X -> a (b)? c   => X -> a X' c
			 *                 => X'-> e
			 *                 => X'-> b
			 * X -> a (b)+ c   => X -> a b X' c
			 *                 => X'-> e
			 *                 => X'-> X' b
			 * X -> a (b)* c   => X -> a X' c
			 *                 => X'-> e
			 *                 => X'-> X' b
			 *
			 * Because of this, we number all rules and all productions and apply the following
			 * scheme for the added rules:
			 * - Rules with the highest bit set describe the added production if neccessary. This is
			 *   never stored in the list in the Syntax class. Note that the remaining part of the
			 *   number is an index to a *production*, not a rule.
			 * - Productions stored in items with the highest bit set describe the epsilon production
			 *   of the added rule. If the two highest bits are set, that means the other added production.
			 */


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

				// Add syntax.
				void STORM_FN add(Rule *rule);
				void STORM_FN add(Production *p);

				// Find the ID of a rule of a production.
				Nat STORM_FN lookup(Rule *rule) const;
				Nat STORM_FN lookup(Production *p) const;

				// Get all productions for a rule.
				RuleInfo ruleInfo(Nat rule) const;

				// Get the name of a rule.
				Str *ruleName(Nat rule) const;

				// Find a production from its id.
				Production *production(Nat id) const;

				// Same syntax as another object?
				Bool STORM_FN sameSyntax(Syntax *o) const;

			private:
				// All known rules and their ID:s.
				Map<Rule *, Nat> *rLookup;

				// All known productions and their ID:s.
				Map<Production *, Nat> *pLookup;

				// All known rules.
				Array<Rule *> *rules;

				// Productions for all rules.
				Array<RuleInfo> *ruleProds;

				// All productions. A production's id can be found in 'lookup'.
				Array<Production *> *productions;

			public:
				// Various masks for rules.
				enum {
					ruleMask    = 0x80000000,
					prodEpsilon = 0x80000000,
					prodRepeat  = 0xC0000000,
					prodMask    = 0xC0000000,
				};

				// Is this a special rule id?
				static inline Bool specialRule(Nat id) {
					return (id & ruleMask) != 0;
				}

				// Get the production id this rule was derived from.
				static Nat baseRule(Nat id) {
					return id & ~Nat(ruleMask);
				}

				// Is this a special production id? Returns either prodEpsilon, prodMask or 0.
				static inline Nat specialProd(Nat id) {
					return id & prodMask;
				}

				// Get the production id.
				static inline Nat baseProd(Nat id) {
					return id & ~Nat(prodMask);
				}

				/**
				 * Ordering.
				 */

				enum Order {
					before,
					same,
					after,
				};

				// Compute which stack item representing a reduction has a higher priority according
				// to the grammar.
				Order execOrder(StackItem *a, StackItem *b) const;
			};

		}
	}
}
