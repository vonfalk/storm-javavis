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
				StackItem(MAYBE(StackItem *) prev, MAYBE(StackItem *) createdBy, ProductionIter iter, Nat inputPos) {
					this->prev = prev;
					this->createdBy = createdBy;
					this->match = null;
					this->iter = iter;
					this->inputPos = inputPos;
					this->state = 0;
					this->data = 0;
				}

				// Create an item that follows 'top' by reading a terminal symbol.
				static StackItem *follow(Engine &e, StackItem *follow, ProductionIter iter, Nat pos) {
					return new (e) StackItem(follow, follow->createdBy, iter, pos);
				}

				// Create an item that branches out from 'top' by reading a nonterminal symbol.
				static StackItem *branch(Engine &e, MAYBE(StackItem *) top, ProductionIter iter, Nat pos) {
					return new (e) StackItem(top, top, iter, pos);
				}

				// Previous item on the stack.
				MAYBE(StackItem *) prev;

				// Pointer to the state that started this sub-production to match (i.e. the 'prev'
				// of the first item in this production).
				MAYBE(StackItem *) createdBy;

				// Syntax tree matched by this item, if any. Only used for nonterminals.
				GcArray<TreePart> *match;

				// Current position in the grammar.
				ProductionIter iter;

				// Current position in the input.
				Nat inputPos;

				// Current state of matching this input. Depends on what 'iter' refers to.
				Nat state;

				// Additional data. Depends on what 'iter' refers to.
				Nat data;
			};

		}
	}
}
