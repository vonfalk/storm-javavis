#pragma once
#include "Production.h"
#include "Core/PODArray.h"

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
			STORM_CTOR State(ProductionIter pos, Nat step, Nat from);
			STORM_CTOR State(ProductionIter pos, Nat step, Nat from, State *prev);
			STORM_CTOR State(ProductionIter pos, Nat step, Nat from, State *prev, State *completed);

			// Position in the production.
			ProductionIter pos;

			// The step where this state belongs.
			Nat step;

			// The step where this production was instantiated.
			Nat from;

			// Previous instance of this state.
			State *prev;

			// State that completed this state. If set, this means that pos->token is a rule token.
			State *completed;

			// Is this a valid state (ie. does it have a valid position?)
			inline Bool valid() const {
				return pos.valid();
			}

			// Get the priority for the rule associated with this state.
			inline Int priority() const {
				return pos.production()->priority;
			}

			// Get the production this state represents.
			inline Production *production() const {
				return pos.production();
			}

			// See if this state is a Rule token.
			inline RuleToken *getRule() const {
				return as<RuleToken>(pos.token());
			}

			// See if this state is a Regex token.
			inline RegexToken *getRegex() const {
				return as<RegexToken>(pos.token());
			}

			// See if this state finishes a production.
			inline Bool finishes(Production *p) const {
				return production() == p
					&& pos.end();
			}

			// Equality, as required by the parser. TODO: adhere to the future standard equal interface.
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
		class StateSet {
			STORM_VALUE;
		public:
			// Create.
			StateSet(Engine &e);

			// Insert a state here if it does not exist. May alter the 'completed' member of an
			// existing state to obey priority rules.
			void STORM_FN push(State *state);

			// Insert a new state (we will allocate it for you if neccessary)
			void STORM_FN push(ProductionIter pos, Nat step, Nat from);
			void STORM_FN push(ProductionIter pos, Nat step, Nat from, State *prev);
			void STORM_FN push(ProductionIter pos, Nat step, Nat from, State *prev, State *completed);

			// Current size of this set.
			Nat STORM_FN count() const { return data->count(); }

			// Get an element in this set.
			State *STORM_FN operator [](nat i) const { return data->at(i); }

		private:
			// State data.
			Array<State *> *data;

			// Size of the array to use when computing production ordering. Should be large enough
			// to cover the majority of productions, otherwise we loose performance.
			typedef PODArray<const State *, 20> StateArray;

			// Ordering.
			enum Order {
				before,
				after,
				none,
			};

			// Compute the execution order of two states when parsed by a top-down parser.
			Order execOrder(const State *a, const State *b) const;

			// Find previous states for a state.
			void prevStates(const State *end, StateArray &to) const;
		};

	}
}
