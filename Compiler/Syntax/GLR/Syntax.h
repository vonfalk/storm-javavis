#pragma once
#include "Core/Array.h"
#include "Core/Map.h"
#include "Core/Set.h"
#include "Core/EnginePtr.h"
#include "Compiler/Syntax/Rule.h"
#include "Compiler/Syntax/Production.h"

namespace storm {
	namespace syntax {
		namespace glr {
			STORM_PKG(lang.bnf.glr);

			class StackItem;
			class Syntax;
			class Item;

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
			class RuleInfo : public Object {
				STORM_CLASS;
			public:
				// Create.
				STORM_CTOR RuleInfo();

				// Count.
				inline Nat STORM_FN count() const { return productions ? productions->count() : 0; }

				// Access elements.
				inline Nat STORM_FN operator[](Nat id) const { return productions ? productions->at(id) : 0; }
				inline Nat at(Nat id) const { return productions ? productions->at(id) : 0; }

				// Add a production.
				void STORM_FN push(Nat id);

				// Add 'regex' to the follow-set of this rule.
				void STORM_FN follows(Regex regex);

				// Add the follow set of 'rule' to ours.
				void STORM_FN follows(Nat rule);

				// Add the first set of 'rule' to the follow set of us.
				void STORM_FN followsFirst(Nat rule);

				// Get the first set of this rule.
				Set<Regex> *STORM_FN first(Syntax *syntax);

				// Get the follow set of this rule.
				Set<Regex> *STORM_FN follows(Syntax *syntax);

			private:
				// All productions for this rule. May be null.
				Array<Nat> *productions;

				// The follow-set of this rule. May be null.
				Set<Regex> *followSet;

				// The follow-set of the rules shall be merged with the follow set in here.
				Set<Nat> *followRules;

				// The first-set of these rules shall be merged with the follow set in here.
				Set<Nat> *firstRules;

				// Computing first and follow sets.
				Bool compFirst;
				Bool compFollows;
			};


			/**
			 * All syntax in a parser.
			 *
			 * Assigns an identifier to each production to make things easier down the line.
			 *
			 * Also, computes the follow set for each non-terminal encountered.
			 */
			class Syntax : public ObjectOn<Compiler> {
				STORM_CLASS;
			public:
				// Create.
				STORM_CTOR Syntax();

				// Add syntax.
				Nat STORM_FN add(Rule *rule);
				Nat STORM_FN add(Production *p);

				// Find the ID of a rule of a production.
				Nat STORM_FN lookup(Rule *rule);
				Nat STORM_FN lookup(Production *p);

				// Get all productions for a rule.
				RuleInfo *ruleInfo(Nat rule);

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
				Array<RuleInfo *> *ruleProds;

				// Productions for repetition rules. Entries in here are created lazily.
				Array<RuleInfo *> *repRuleProds;

				// All productions. A production's id can be found in 'lookup'.
				Array<Production *> *productions;

				// Add the follow-set of a production.
				void addFollows(Production *p);

				// Add the thing 'Item' refers to to the rule info 'into'.
				void addFollows(RuleInfo *into, const Item &pos);

			public:
				// Various masks for rules.
				enum {
					ruleMask    = 0xC0000000,
					ruleRepeat  = 0x80000000,
					ruleESkip   = 0x40000000,
					prodEpsilon = 0x80000000,
					prodESkip   = 0x40000000,
					prodRepeat  = 0xC0000000,
					prodMask    = 0xC0000000,
				};

				// Is this a special rule id? Returns ruleRepeat or ruleESkip.
				static inline Nat specialRule(Nat id) {
					return id & ruleMask;
				}

				// Get the production id this rule was derived from.
				static Nat baseRule(Nat id) {
					return id & ~Nat(ruleMask);
				}

				// Is this a special production id? Returns either prodEpsilon, prodESkip, prodMask or 0.
				static inline Nat specialProd(Nat id) {
					return id & prodMask;
				}

				// Get the production id.
				static inline Nat baseProd(Nat id) {
					return id & ~Nat(prodMask);
				}

			};

		}
	}
}
