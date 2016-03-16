#include "stdafx.h"
#include "State.h"
#include "Exception.h"

namespace storm {
	namespace syntax {

#ifdef DEBUG
		bool debugParser = false;
#endif

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
					if (execOrder(&state, c) == before) {
						// Note: as * and + are greedy, we might end up in a case where we deem it
						// beneficial to create a loop of states. Avoid that! Note: this is only
						// possible for multiple zero-length rules and therefore only states in the
						// current step need to be considered.
						for (const State *at = c->prev; at && at->step == c->step; at = at->prev) {
							// Loop found, avoid it!
							if (at == c)
								return;
						}

						// No loops. Go on!
						*c = state;
					}

					return;
				}
			}

			State *s = alloc.alloc(state);
			data.push_back(s);
		}

		StateSet::Order StateSet::execOrder(const State *a, const State *b) const {
			// Invalid states have no ordering.
			if (!a)
				return none;
			if (!b)
				return none;

			// Same state, no difference.
			if (a == b)
				return none;

			// The one created earliest in the sequence goes before.
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
			for (const State *now = from; now->prev; now = now->prev)
				to.push(now);
			to.reverse();
		}

	}
}
