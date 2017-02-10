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

				// All productions for this rule. May be null.
				Array<Nat> *productions;

				// Safely add a production.
				void push(Engine &e, Nat id);
			};


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
				Map<Production *, Nat> *prods;

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
			};


			/**
			 * Item set.
			 *
			 * Ordered set of fixed-sized items. Designed for low memory overhead for a small amount
			 * of items.
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

				// Compare.
				Bool STORM_FN operator ==(const ItemSet &o) const;
				Bool STORM_FN operator !=(const ItemSet &o) const;

				// Hash.
				Nat STORM_FN hash() const;

				// Push an item.
				void push(Engine &e, Item item);

			private:
				// Data. Might be null.
				GcArray<Item> *data;

				// Growth factor.
				enum {
					grow = 10
				};
			};

			// Push items.
			void STORM_FN push(EnginePtr e, ItemSet &to, Item item);


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
			};


			/**
			 * One row in the LR table.
			 */
			class State : public ObjectOn<Compiler> {
				STORM_CLASS;
			public:
				// Create.
				STORM_CTOR State();

				// All actions in here.
				Array<ShiftAction *> *actions;
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

			private:
				// All syntax.
				Syntax *syntax;

				// Lookup of state-sets to state id.
				Map<ItemSet, Nat> *lookup;

				// States.
				Array<State *> *states;
			};

		}
	}
}
