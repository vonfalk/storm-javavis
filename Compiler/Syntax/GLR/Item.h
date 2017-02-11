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

		}
	}
}
