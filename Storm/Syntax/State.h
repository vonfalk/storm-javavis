#pragma once
#include "Option.h"
#include "Utils/PreArray.h"

namespace storm {
	namespace syntax {

		/**
		 * State used during parsing (in Parser.h/cpp). Contains a location into an option, a step
		 * where the option was instantiated, pointer to previous step, and optionally the step that
		 * was completed.
		 */
		class State {
		public:
			// Create.
			State();
			State(const OptionIter &pos, nat from, State *prev = null, State *completed = null);

			// Position in the rule.
			OptionIter pos;

			// In which step was this rule instantiated?
			nat from;

			// Previous instance of this state; ie. where pos is one step ahead of the pos here.
			State *prev;

			// Which state was completed in order to produce this state? This means that the token
			// of prev->pos is a RuleToken.
			State *completed;

			// Get the priority for the rule associated to this state.
			inline Int priority() const {
				return pos.optionPtr()->priority;
			}

			// See if this state finishes 'option'.
			bool finishes(Par<Option> option) const;

			// See if this state is a Rule token.
			RuleToken *getRule() const;

			// See if this state is a Regex token.
			RegexToken *getRegex() const;

			// Equality, as required by the parser.
			inline bool operator ==(const State &o) const {
				return pos == o.pos
					&& from == o.from;
			}

			inline bool operator !=(const State &o) const {
				return !(*this == o);
			}
		};

		wostream &operator <<(wostream &to, const State &s);
		wostream &operator <<(wostream &to, const State *s);


		/**
		 * Allocator for State objects. Optimized for allocating a large number of States, which
		 * will be freed at the same time.
		 */
		class StateAlloc : NoCopy {
		public:
			// Create.
			StateAlloc();

			// Destroy. Frees all memory.
			~StateAlloc();

			// Clear. Frees all memory.
			void clear();

			// How much memory is used (entries)?
			nat count() const;

			// Allocate one State.
			State *alloc();
			State *alloc(const State &copy);
			State *alloc(const OptionIter &pos, nat from, State *prev = null, State *completed = null);

		private:
			// # of allocations in each pool.
			enum {
				poolCount = 1024
			};

			// One allocation pool.
			typedef State *Pool;

			// Current pool.
			Pool current;

			// First free element in the current pool.
			nat firstFree;

			// Old pools which needs freeing later on.
			vector<Pool> used;

			// Allocate some memory, but do not create an object there.
			void *rawAlloc();

			// Allocate a new pool.
			void newPool();
		};


		/**
		 * Ordered set of states. Supports proper ordering by priority.
		 */
		class StateSet {
		public:
			// Insert a state here if it does not exist. May alter the 'completed' member of an
			// existing state to obey the priority rules.
			void push(const State &state, StateAlloc &alloc);

			// Current size of this set.
			inline nat count() const { return data.size(); }

			// Get an element in this set.
			inline State *operator [](nat i) { return data[i]; }

		private:
			// State data.
			vector<State *> data;

			// Size of the array for computing ordering. Should be large enough to accomodate the
			// common length of rules to not lose too much performance.
			typedef PreArray<State *, 20> StateArray;

			// Ordering.
			enum Order {
				before,
				after,
				none,
			};

			// Compute the execution order of two states when parsed by a top-down parser.
			Order execOrder(State *a, State *b) const;

			// Find previous states for a state.
			void prevStates(State *end, StateArray &to) const;
		};

	}
}
