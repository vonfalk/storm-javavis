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
				StackItem(Nat itemId, MAYBE(StackItem *) prev, ProductionIter iter, Nat inputPos) {
					this->prev = prev;
					this->iter = iter;
					this->itemId = itemId;
					this->inputPos = inputPos;
				}

				// Previous item on this stack.
				MAYBE(StackItem *) prev;

				// Current position in the grammar.
				ProductionIter iter;

				// Production id, for disambiguating productions on the same position.
				Nat itemId;

				// Current position in the input.
				Nat inputPos;

				// Comparison in the priority queue.
				Bool operator <(const StackItem &other) const {
					if (inputPos != other.inputPos)
						return inputPos < other.inputPos;
					else
						return itemId < other.itemId;
				}
			};

		}
	}
}
