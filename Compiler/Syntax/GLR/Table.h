#pragma once
#include "Core/Map.h"
#include "Core/Array.h"
#include "Core/GcArray.h"
#include "Core/EnginePtr.h"
#include "Compiler/Syntax/Rule.h"
#include "Compiler/Syntax/Production.h"

namespace storm {
	namespace syntax {
		namespace glr {
			STORM_PKG(lang.bnf.glr);

			/**
			 * Various data structures used by the GLR parser internally.
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

				// All known rules.
				Map<Rule *, RuleInfo> *rules;

				// All known productions and their ID:s.
				Map<Production *, Nat> *lookup;

				// All productions. A production's id can be found in 'lookup'.
				Array<Production *> *productions;

				// Add syntax.
				void STORM_FN add(Rule *rule);
				void STORM_FN add(Production *type);

				// Same syntax as another object?
				Bool STORM_FN sameSyntax(Syntax *o);
			};


			/**
			 * Item in the LR tables. Has has functions and is therefore easy to look for.
			 *
			 * TODO: We could easily store this as one single 32-bit number, slightly skewed from
			 * 50/50 thouhgh, we want to support slightly more than 64k productions.
			 */
			class Item {
				STORM_VALUE;
			public:
				// Create.
				STORM_CTOR Item(Syntax *world, ProductionIter iter);

				// The production id.
				Nat id;

				// The position inside the production.
				Nat pos;

				// Create an iterator.
				ProductionIter STORM_FN iter(Syntax *s) const;

				// Hash function.
				Nat STORM_FN hash() const;

				// Equality.
				inline Bool STORM_FN operator ==(Item o) const {
					return id == o.id
						&& pos == o.pos;
				}

				inline Bool STORM_FN operator !=(Item o) const {
					return !(*this == o);
				}

				// Lexiographic ordering.
				inline Bool STORM_FN operator <(Item o) const {
					if (id != o.id)
						return id < o.id;
					return pos < o.pos;
				}

				// To string.
				Str *STORM_FN toS(Syntax *syntax) const;
			};

			// Plain to string (no syntax lookup possible).
			StrBuf &STORM_FN operator <<(StrBuf &to, Item item);


			/**
			 * Item set.
			 *
			 * Ordered set of fixed-sized items. Designed for low memory overhead for a small amount
			 * of items.
			 *
			 * Note: The underlying data is partly shared, so do not modify these unless you created
			 * them!
			 */
			class ItemSet {
				STORM_VALUE;
			public:
				// Create.
				STORM_CTOR ItemSet();

				// Element access.
				inline Nat STORM_FN count() const { return data ? data->filled : 0; }
				inline const Item &at(Nat id) const { return data->v[id]; }
				Item STORM_FN operator [](Nat id) const;

				// Does this set contain a specific element?
				Bool STORM_FN has(Item i) const;

				// Compare.
				Bool STORM_FN operator ==(const ItemSet &o) const;
				Bool STORM_FN operator !=(const ItemSet &o) const;

				// Hash.
				Nat STORM_FN hash() const;

				// Push an item.
				Bool push(Engine &e, Item item);

				// Push an iterator if it is valid. Returns 'true' if inserted.
				Bool STORM_FN push(Syntax *syntax, ProductionIter iter);

				// Expand all nonterminal symbols in this item set.
				ItemSet STORM_FN expand(Syntax *syntax) const;

				// To string.
				Str *STORM_FN toS(Syntax *syntax) const;

			private:
				// Data. Might be null.
				GcArray<Item> *data;

				// Growth factor.
				enum {
					grow = 10
				};

				// Find the index of the first element greater than or equal to 'find' in O(log n).
				Nat itemPos(Item find) const;
			};

			// Push items.
			Bool STORM_FN push(EnginePtr e, ItemSet &to, Item item);

			// Plain to string (no syntax lookup possible).
			StrBuf &STORM_FN operator <<(StrBuf &to, ItemSet itemSet);


			/**
			 * One shift entry in the action table.
			 */
			class ShiftAction : public ObjectOn<Compiler> {
				STORM_CLASS;
			public:
				// Create.
				STORM_CTOR ShiftAction(Regex regex, Nat state);

				// Regular expression to match.
				Regex regex;

				// Go to this state.
				Nat state;

				// To string.
				virtual void STORM_FN toS(StrBuf *to) const;
			};


			/**
			 * One row in the LR table.
			 */
			class State : public ObjectOn<Compiler> {
				STORM_CLASS;
			public:
				// Create.
				STORM_CTOR State(ItemSet items);

				// The item set of this state.
				ItemSet items;

				// All shift actions in here. If the array is 'null', the actions need to be created.
				MAYBE(Array<ShiftAction *> *) actions;

				// The goto table. If 'null' it needs to be created. (Note: can not be named goto...)
				MAYBE(Map<Rule *, Nat> *) rules;

				// Reduce these productions in this state (we're LR 0, so always do that).
				// TODO: Sort these so that the highest priority production comes first.
				MAYBE(Array<Nat> *) reduce;

				// To string.
				virtual void STORM_FN toS(StrBuf *to) const;
				void STORM_FN toS(StrBuf *to, Syntax *syntax) const;
			};


			/**
			 * Lazily generated parse table (currently LR(0)).
			 *
			 * Basically associates item sets to indices and generates new indices on demand.
			 */
			class Table : public ObjectOn<Compiler> {
				STORM_CLASS;
			public:
				// Create.
				STORM_CTOR Table(Syntax *syntax);

				// Is the table empty?
				Bool STORM_FN empty() const;

				// Find the index of a state set.
				Nat STORM_FN state(ItemSet s);

				// Get the actual state from an index.
				State *STORM_FN state(Nat id);

				// To string.
				virtual void STORM_FN toS(StrBuf *to) const;

			private:
				// All syntax.
				Syntax *syntax;

				// Lookup of item-sets to state id.
				Map<ItemSet, Nat> *lookup;

				// States.
				Array<State *> *states;

				// Compute the contents of a state.
				void fill(State *state);

				// Create the actions for a state.
				ShiftAction *createShift(Nat start, ItemSet items, Array<Bool> *used, RegexToken *regex);
				Nat createGoto(Nat start, ItemSet items, Array<Bool> *used, Rule *rule);
			};

		}
	}
}
