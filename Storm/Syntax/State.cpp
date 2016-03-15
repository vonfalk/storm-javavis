#include "stdafx.h"
#include "State.h"
#include "Exception.h"

namespace storm {
	namespace syntax {

		State::State() : from(0), prev(null), completed(null) {}

		State::State(const OptionIter &pos, nat step, nat from, State *prev, State *completed) :
			pos(pos), step(step), from(from), prev(prev), completed(completed) {}

		bool State::finishes(Par<Option> option) const {
			return pos.optionPtr() == option.borrow()
				&& pos.end();
		}

		RuleToken *State::getRule() const {
			if (pos.end())
				return null;
			return as<RuleToken>(pos.tokenPtr());
		}

		RegexToken *State::getRegex() const {
			if (pos.end())
				return null;
			return as<RegexToken>(pos.tokenPtr());
		}

		wostream &operator <<(wostream &to, const State &s) {
			to << toHex(&s) << L":{" << s.pos << L", " << s.from << L", " << toHex(s.prev) << L", " << toHex(s.completed) << L"}";
			return to;
		}

		wostream &operator <<(wostream &to, const State *s) {
			return to << *s;
		}


		/**
		 * Allocator.
		 */

		StateAlloc::StateAlloc() : current(null), firstFree(0) {}

		StateAlloc::~StateAlloc() {
			clear();
		}

		nat StateAlloc::count() const {
			return used.size() * poolCount + firstFree;
		}

		State *StateAlloc::alloc() {
			void *m = rawAlloc();
			return new (m) State();
		}

		State *StateAlloc::alloc(const State &copy) {
			void *m = rawAlloc();
			return new (m) State(copy);
		}

		State *StateAlloc::alloc(const OptionIter &pos, nat step, nat from, State *prev, State *completed) {
			void *m = rawAlloc();
			return new (m) State(pos, step, from, prev, completed);
		}

		void *StateAlloc::rawAlloc() {
			if (current == null || firstFree >= poolCount)
				newPool();

			return &current[firstFree++];
		}

		void StateAlloc::newPool() {
			if (current)
				used.push_back(current);

			firstFree = 0;
			current = (Pool)malloc(sizeof(State) * poolCount);
			if (!current)
				throw InternalError(L"Out of memory while parsing.");
		}

		void StateAlloc::clear() {
			// Free old pools.
			for (nat i = 0; i < used.size(); i++) {
				for (nat j = 0; j < poolCount; j++)
					used[i][j].~State();

				free(used[i]);
			}

			// Free the current pool.
			if (current) {
				for (nat i = 0; i < firstFree; i++)
					current[i].~State();

				free(current);
			}

			used.clear();
			current = null;
			firstFree = 0;
		}

		/**
		 * Set.
		 */

		void StateSet::push(const State &state, StateAlloc &alloc) {
			if (!state.pos.valid())
				return;

			for (nat i = 0; i < data.size(); i++) {
				State *c = data[i];
				if (*c == state) {
					// Found it already, shall we update the existing one?
					if (execOrder(state.completed, c->completed) == before)
						*c = state;
					return;
				}
			}

			State *s = alloc.alloc(state);
			data.push_back(s);
		}

		StateSet::Order StateSet::execOrder(State *a, State *b) const {
			// Invalid states have no ordering.
			if (!a)
				return none;
			if (!b)
				return none;

			// The one created earliest in the sequence goes before.
			// NOTE: This is modified slightly from the last incarnation of the parser!
			if (a->from != b->from)
				return (a->from < b->from) ? before : after;

			// Highest priority goes first.
			if (a->priority() != b->priority())
				return (a->priority() > b->priority()) ? before : after;

			// If they are different options and have the same priority, the ordering is undefined.
			if (a->pos.optionPtr() != b->pos.optionPtr())
				return none;

			// Find out the ordering of the respective parts by a simple lexiographic ordering.
			StateArray aStates, bStates;
			prevStates(a, aStates);
			prevStates(b, bStates);

			nat to = min(aStates.count(), bStates.count());
			for (nat i = 0; i < to; i++) {
				Order order = execOrder(aStates[i], bStates[i]);
				if (order != none)
					return order;
			}

			// The shortest one wins if they are equal this far.
			if (aStates.count() < bStates.count())
				return before;
			else
				return after;
		}

		void StateSet::prevStates(State *from, StateArray &to) const {
			for (State *now = from->prev; now; now = now->prev)
				to.push(now);
			to.reverse();
		}

	}
}
