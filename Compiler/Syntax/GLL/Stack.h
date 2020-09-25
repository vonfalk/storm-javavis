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
				StackItem(Nat itemId, Bool first, MAYBE(StackItem *) prev, ProductionIter iter, Nat inputPos) {
					this->prev = prev;
					this->iter = iter;
					this->data1 = itemId;
					this->data2 = (inputPos << 1) | Nat(first);
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

				// State id, for disambiguating productions on the same position.
				Nat itemId() const {
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
						return itemId() < other.itemId();
				}
			};

		}
	}
}
