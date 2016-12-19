#include "stdafx.h"
#include "State.h"

namespace storm {
	namespace syntax {

		State::State() : from(0) {}

		State::State(ProductionIter pos, Nat step, Nat from) : pos(pos), step(step), from(from) {}

		State::State(ProductionIter pos, Nat step, Nat from, State *prev)
			: pos(pos), step(step), from(from), prev(prev) {}

		State::State(ProductionIter pos, Nat step, Nat from, State *prev, State *completed)
			: pos(pos), step(step), from(from), prev(prev), completed(completed) {}

		void State::toS(StrBuf *to) const {
			*to << hex(this) << L":{" << pos << L", " << from << L", " << hex(prev) << L", " << hex(completed) << L"}";
		}


		StateSet::StateSet(Engine &e) : data(new (e) Array<State *>()) {}

		void StateSet::push(State *s) {
			if (!s->valid())
				return;

			// TODO: Optimize this somehow. A lot of time is spent here!
			Nat c = data->count(); // tells the compiler we will not change the size.
			for (nat i = 0; i < c; i++) {
				State *c = data->at(i);
				if (*c == *s) {
					// Generated this state already, shall we update the existing one?

					// Keep the largest in a lexiographic ordering.
					if (execOrder(s, c) != before)
						return;

					data->at(i) = s;
					return;
				}
			}

			data->push(s);
		}

		void StateSet::push(ProductionIter pos, Nat step, Nat from) {
			push(pos, step, from, null, null);
		}

		void StateSet::push(ProductionIter pos, Nat step, Nat from, State *prev) {
			push(pos, step, from, prev, null);
		}

		void StateSet::push(ProductionIter pos, Nat step, Nat from, State *prev, State *completed) {
			if (!pos.valid())
				return;

			push(new (data) State(pos, step, from, prev, completed));
		}

		StateSet::Order StateSet::execOrder(const State *a, const State *b) const {
			// Invalid states have no ordering.
			if (!a || !b)
				return none;

			// Same state, realize that quickly!
			if (a == b)
				return none;

			// Check which has the highest priority.
			if (a->priority() != b->priority())
				return (a->priority() > b->priority()) ? before : after;

			// If they are different productions and noone has a higher priority than the other, the
			// ordering is undefined.
			if (a->production() != b->production())
				return none;

			// Order them lexiographically to see which has the highest priority!
			StateArray aStates(data->engine());
			prevStates(a, aStates);
			StateArray bStates(data->engine());
			prevStates(b, bStates);

			nat to = min(aStates.count(), bStates.count());
			for (nat i = 0; i < to; i++) {
				Order order = execOrder(aStates[i]->completed, bStates[i]->completed);
				if (order != none)
					return order;
			}

			// Pick the longer one in case they are equal so far. This makes * and + greedy.
			if (aStates.count() > bStates.count()) {
				return before;
			} else if (aStates.count() < bStates.count()) {
				return after;
			}

			// The rules are equal as far as we are concerned.
			return none;
		}

		void StateSet::prevStates(const State *from, StateArray &to) const {
			to.clear();
			for (const State *now = from; now->prev; now = now->prev)
				to.push(now);

			to.reverse();
		}


	}
}
