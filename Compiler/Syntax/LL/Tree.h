#pragma once
#include "Core/GcArray.h"

namespace storm {
	namespace syntax {
		namespace ll {
			STORM_PKG(lang.bnf.ll);

#pragma warning (push)
#pragma warning (disable: 4293) // Warning about shifts in dead code on 32-bit systems.

			// Encode rule+production pair. This allows for ~128k rules and ~32k productions in each rule.
			inline size_t encodeRuleProd(Nat rule, Nat prod) {
				if (sizeof(size_t) == 4) {
					return rule << 15 | prod;
				} else {
					return size_t(rule) << 32 | prod;
				}
			}

			// Decode rule+production pair.
			inline Nat decodeRule(size_t combined) {
				if (sizeof(size_t) == 4) {
					return combined >> 15;
				} else {
					return Nat(combined >> 32);
				}
			}
			inline Nat decodeProd(size_t combined) {
				if (sizeof(size_t) == 4) {
					return combined & 0x7FFF;
				} else {
					return Nat(combined & 0xFFFFFFFF);
				}
			}

#pragma warning (pop)


			/**
			 * Minimal representation of a match that can be turned into a parse tree at a later
			 * stage.
			 *
			 * Each instance of this value represents either a matched terminal or nonterminal symbol.
			 * As such, an array of these represents how an entire production was matched.
			 *
			 * In case an instance represents a terminal symbol, "match" is null.
			 *
			 * In case an instance represents a nonterminal symbol, "match" represents the
			 * nonterminal match, and "match->filled" contains the ID of the matched rule and
			 * production.
			 */
			class TreePart {
				STORM_VALUE;
			public:
				// Create.
				TreePart() {}
				TreePart(Nat startPos) {
					this->startPos = startPos;
					this->match = null;
				}
				// We assume that 'filled' is set up with 'encodeRuleProd' as appropriate.
				TreePart(GcArray<TreePart> *match, Nat startPos) {
					this->startPos = startPos;
					this->match = match;
				}

				// Starting position of the match.
				Nat startPos;

				// Pointer to details for a non-terminal, if applicable.
				GcArray<TreePart> *match;

				// Is this a terminal symbol?
				Bool isTerminal() const {
					return match == null;
				}

				// Is this a nonterminal symbol?
				Bool isRule() const {
					return match != null;
				}

				// Get the rule ID. Assumes 'match' is not null.
				// Nat ruleId() const {
				// 	return decodeRule(match->filled);
				// }

				// Get the production ID. Assumes 'match' is not null.
				Nat productionId() const {
					return decodeProd(match->filled);
				}
			};

			typedef GcArray<TreePart> Tree;

		}
	}
}
