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
					this->prev = prev;
					this->iter = iter;
					this->data1 = 0;
					this->data2 = (inputPos << 1) | Nat(first);

					if (prev)
						this->data1 = prev->depth();
					if (first)
						this->data1++;
				}

				// Previous item on this stack.
				MAYBE(StackItem *) prev;

				// What did this production match? Used only for nonterminals.
				GcArray<TreePart> *match;

				// Current position in the grammar.
				ProductionIter iter;

				// Data. Contains 'itemId'.
				Nat data1;

				// Data. Contains 'inputPos' and 'first'.
				Nat data2;

				// How "deep" in the grammar this production is. Used to ensure that we finish all
				// possible states that may complete a particular state before proceeding.
				Nat depth() const {
					return data1;
				}

				// Is this the first state in a production?
				Nat first() const {
					return (data2 & 0x1) != 0;
				}

				// Current position in the input.
				Nat inputPos() const {
					return data2 >> 1;
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
			};

		}
	}
}
