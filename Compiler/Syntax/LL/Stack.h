#pragma once
#include "Compiler/Syntax/Production.h"

namespace storm {
	namespace syntax {
		namespace ll {
			STORM_PKG(lang.bnf.ll);

			/**
			 * An item on the LL stack.
			 */
			class StackItem : public Object {
				STORM_CLASS;
			public:
				// Create.
				StackItem(MAYBE(StackItem *) prev, ProductionIter iter, Nat inputPos) {
					this->prev = prev;
					this->iter = iter;
					this->inputPos = inputPos;
					this->state = 0;
					this->data = 0;
				}

				// Previous item on the stack.
				MAYBE(StackItem *) prev;

				// Current position in the grammar.
				ProductionIter iter;

				// Current position in the input.
				Nat inputPos;

				// Current state of matching this input. Depends on what 'iter' refers to.
				Nat state;

				// Data used by the matching. Depends on what 'iter' refers to.
				Nat data;
			};

		}
	}
}
