#pragma once
#include "Production.h"

namespace storm {
	namespace syntax {
		STORM_PKG(lang.bnf);

		/**
		 * State used during parsing (in Parser.h/cpp). Contains a location into a production, a
		 * step where the option was instantiated, a pointer to the previous step and optionally the
		 * step that was completed.
		 *
		 * TODO: Allocate these on a separate GC-pool?
		 */
		class State : public Object {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR State();
			STORM_CTOR State(ProductionIter pos, Nat from, State *prev, State *completed);

			// Position in the production.
			ProductionIter pos;

			// The step where this production was instantiated.
			Nat from;

			// Previous instance of this state.
			State *prev;

			// State that completed this state. If set, this means that pos->token is a rule token.
			State *completed;

			// Get the priority for the rule associated with this state.
			inline Int priority() const {
				return pos.production()->priority;
			}

			// See if this state is a Rule token.
			inline RuleToken *getRule() const {
				return as<RuleToken>(pos.token());
			}

			// See if this state is a Regex token.
			inline RegexToken *getRegex() const {
				return as<RegexToken>(pos.token());
			}

			// Equality, as required by the parser.
			inline Bool STORM_FN operator ==(const State &o) const {
				return pos == o.pos
					&& from == o.from;
			}

			inline Bool STORM_FN operator !=(const State &o) const {
				return !(*this == o);
			}

			// To string.
			void STORM_FN toS(StrBuf *to) const;
		};


		/**
		 * Ordered set of states. Supports proper ordering by priority.
		 */
		class StateSet : public Object {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR StateSet();

			// Insert a state here if it does not exist. May alter the 'completed' member of an
			// existing state to obey priority rules.
			// void push(const State &state);

			// Current size of this set.
			// nat count() const;

			// Get an element in this set.
			// State &operator [](nat i);

		private:
		};

	}
}
