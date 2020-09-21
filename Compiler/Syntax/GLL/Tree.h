#pragma once
#include "Core/GcArray.h"

namespace storm {
	namespace syntax {
		namespace gll {
			STORM_PKG(lang.bnf.gll);


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
				// We assume that 'filled' is set up with the production matched by 'match'.
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

				// Get the production ID. Assumes 'match' is not null.
				Nat productionId() const {
					return Nat(match->filled);
				}
			};

			typedef GcArray<TreePart> Tree;

		}
	}
}
