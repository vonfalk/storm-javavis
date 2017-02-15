#pragma once
#include "Core/Object.h"
#include "Core/GcArray.h"
#include "Core/Set.h"
#include "Compiler/Thread.h"

namespace storm {
	namespace syntax {
		namespace glr {
			STORM_PKG(lang.bnf.glr);

			/**
			 * Stack items in the GLR parser.
			 *
			 * TODO: It could be useful to merge these into larger chunks and use one-dimensional
			 * ID:s for pointers to them.
			 */
			class StackItem : public Object {
				STORM_CLASS;
			public:
				// Create.
				STORM_CTOR StackItem(Nat state);
				STORM_CTOR StackItem(Nat state, StackItem *prev);

				// State at this point in the stack.
				Nat state;

				// Previous item in the stack.
				MAYBE(StackItem *) prev;

				// More previous states? Forms a linked list of multiple StackItem nodes at the same
				// level (we ignore 'state' and 'reduced' there) of more previous items.
				MAYBE(StackItem *) morePrev;

				// The stack caused this item by reduction.
				MAYBE(StackItem *) reduced;

				// If 'reduced': the production that was reduced to produce the current item.
				Nat reducedId;

				// Insert a node in the 'morePrev' chain if it is not already there. Returns 'true' if it was inserted.
				Bool STORM_FN insert(StackItem *item);

				// Equality check and hashing.
				virtual Bool STORM_FN equals(Object *o) const;
				virtual Nat STORM_FN hash() const;

				// Output.
				virtual void STORM_FN toS(StrBuf *to) const;
			};


			/**
			 * Set of future stack sets. Implements a sequence of sets where lookup is constant time
			 * and popping the front is also constant time. Any access extends the structure to the
			 * desired length.
			 */
			class FutureStacks : public ObjectOn<Compiler> {
				STORM_CLASS;
			public:
				STORM_CTOR FutureStacks();

				// Get the topmost set.
				MAYBE(Set<StackItem *> *) STORM_FN top();

				// Pop the topmost set, shifting all other indices one step.
				void STORM_FN pop();

				// Insert an item at location 'pos'. The top is at location 0.
				void STORM_FN put(Nat pos, StackItem *item);

			private:
				// Storage. Used as a circular queue. Size is always a power of two.
				GcArray<Set<StackItem *> *> *data;

				// First element position.
				Nat first;

				// Grow to fit at least 'n' elements.
				void grow(Nat n);

				// Number of elements in 'data' currently.
				inline Nat count() const { return data ? data->count : 0; }

				// Wrap 'n' to the size of data.
				inline Nat wrap(Nat n) const { return data ? (n & (data->count - 1)) : 0; }
			};


		}
	}
}
