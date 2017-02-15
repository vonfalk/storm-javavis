#pragma once
#include "Syntax.h"
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
			 * Item in the LR tables. Has has functions and is therefore easy to look for.
			 *
			 * See 'syntax.h' for notes on the identifiers used here!
			 */
			class Item {
				STORM_VALUE;
			public:
				// Create an item representing the first position of 'p'.
				STORM_CTOR Item(Syntax *world, Production *p);

				// Create from a production-id in 'world'.
				STORM_CTOR Item(Syntax *world, Nat production);

				// The production id.
				Nat id;

				// The position inside the production. Note: position 0xFFFF
				Nat pos;

				// Get the rule.
				Nat STORM_FN rule(Syntax *syntax) const;

				// Get the number of tokens in this production.
				Nat STORM_FN length(Syntax *syntax) const;

				// Get the next item in the sequence.
				Item STORM_FN next(Syntax *syntax) const;

				// Is this item at the end of the production?
				Bool STORM_FN end() const;

				// Is this item's next position a token?
				Bool STORM_FN isRule(Syntax *syntax) const;

				// Get the next token. Only available if 'isRule' is true.
				Nat STORM_FN nextRule(Syntax *syntax) const;

				// Get the next regex. Only available if 'isRule' is false.
				Regex STORM_FN nextRegex(Syntax *syntax) const;

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

			private:
				// Create with raw id and position.
				Item(Nat id, Nat pos);

				// The pos used when we are at the end.
				static Nat endPos;

				// The pos used when we are at the special token.
				static Nat specialPos;

				// Compute the first and next positions in a production.
				static Nat firstPos(Production *p);
				static Nat nextPos(Production *p, Nat pos);
				static Nat firstRepPos(Production *p);
				static Nat nextRepPos(Production *p, Nat pos);

				// Output helpers.
				typedef Nat (*FirstFn)(Production *);
				typedef Nat (*NextFn)(Production *, Nat);
				void output(StrBuf *to, Production *p, Nat mark, FirstFn first, NextFn next) const;
				void outputToken(StrBuf *to, Production *p, Nat pos) const;
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
				Bool STORM_FN operator ==(ItemSet o) const;
				Bool STORM_FN operator !=(ItemSet o) const;

				// Hash.
				Nat STORM_FN hash() const;

				// Push an item. Returns 'true' if inserted.
				Bool push(Engine &e, Item item);

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

		}
	}
}
