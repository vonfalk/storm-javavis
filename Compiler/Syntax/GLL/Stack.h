#pragma once
#include "Compiler/Syntax/Production.h"
#include "Tree.h"

namespace storm {
	namespace syntax {
		namespace gll {
			STORM_PKG(lang.bnf.gll);

			/**
			 * An item on the LL stack.
			 */
			class StackItem : public Object {
				STORM_CLASS;
			public:
				// Create.
				StackItem(Bool first, MAYBE(StackItem *) prev, ProductionIter iter, Nat inputPos) {
					this->prevData = prev;
					this->iter = iter;
					this->data1 = 0;
					this->data2 = (inputPos << 2) | (Nat(first) << 1);

					if (prev)
						this->data1 = prev->depth();
					if (first)
						this->data1++;
				}

				// What did this production match? Used only for nonterminals.
				GcArray<TreePart> *match;

				// Current position in the grammar.
				ProductionIter iter;

				// Get number of previous nodes.
				Nat prevCount() const {
					if (!multi())
						return prevData ? 1 : 0;
					else
						return ((GcArray<StackItem *> *)prevData)->filled;
				}

				// Get the previous node at id.
				StackItem *prev(Nat id) const {
					if (!multi())
						return (StackItem *)prevData;
					else
						return ((GcArray<StackItem *> *)prevData)->v[id];
				}

				// Add another previous node.
				void prevPush(StackItem *item) {
					if (!prevData) {
						prevData = item;
					} else if (!multi()) {
						Engine &e = item->engine();
						GcArray<StackItem *> *array = runtime::allocArray<StackItem *>(e, &pointerArrayType, 10);
						array->filled = 2;
						array->v[0] = (StackItem *)prevData;
						array->v[1] = item;
						prevData = array;
						data2 |= 0x1;
					} else {
						GcArray<StackItem *> *array = (GcArray<StackItem *> *)prevData;
						if (array->filled >= array->count) {
							Engine &e = item->engine();
							GcArray<StackItem *> *n = runtime::allocArray<StackItem *>(e, &pointerArrayType, array->count * 2);
							memcpy(n->v, array->v, array->filled*sizeof(StackItem *));
							array = n;
							prevData = n;
						}

						array->v[array->filled++] = item;
					}
				}

				// How "deep" in the grammar this production is. Used to ensure that we finish all
				// possible states that may complete a particular state before proceeding.
				Nat depth() const {
					return data1;
				}

				// Is this the first state in a production?
				Bool first() const {
					return (data2 & 0x2) != 0;
				}

				// Current position in the input.
				Nat inputPos() const {
					return data2 >> 2;
				}

				// Comparison in the priority queue.
				Bool operator <(const StackItem &other) const {
					if (inputPos() != other.inputPos())
						return inputPos() < other.inputPos();
					else
						// When de-duplicating states, this should maybe be the other way, so that
						// we exhaust all possibilities towards the root of the tree first, for
						// maximum deduplication possibilities, so that our priority comparison can
						// be used for maximum benefit.
						return depth() > other.depth();
				}

			private:
				// Either a stack item, or a pointer to an array of stack items, depending on if
				// "multi" is true or false.
				UNKNOWN(PTR_GC) void *prevData;

				// Data. Contains 'itemId'.
				Nat data1;

				// Data. Contains 'inputPos' (high 30 bits), 'first' (bit 1), and 'multi' (bit 0).
				Nat data2;


				// Containing multiple "prev"?
				Bool multi() const {
					return (data2 & 0x1) != 0;
				}
			};

		}
	}
}
