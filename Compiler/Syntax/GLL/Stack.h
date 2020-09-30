#pragma once
#include "Compiler/Syntax/Production.h"
#include "Tree.h"

namespace storm {
	namespace syntax {
		namespace gll {
			STORM_PKG(lang.bnf.gll);

			class StackFirst;
			class StackRule;

			/**
			 * An item on the LL stack.
			 *
			 * This is used for states that are not the first one in a production. For those, we use
			 * the subclass StackFirst instead.
			 *
			 * The main difference is that StackFirst need to keep track of a bit more state (if
			 * this production was finished yet, and possibly multiple previous nodes).
			 */
			class StackItem : public Object {
				STORM_CLASS;
			public:
				// Create.
				StackItem(MAYBE(StackItem *) prev, ProductionIter iter, Nat inputPos)
					: prev(prev), iter(iter), data1(inputPos << 2) {
					if (prev)
						myDepth = prev->depth();
				}

				// Previous item, if any. Note: If this is a StackFirst, then there may be more of
				// these accessible through the interface in StackMore.
				MAYBE(StackItem *) prev;

				// Current position in the grammar.
				ProductionIter iter;

				// Input position.
				Nat inputPos() const {
					return data1 >> 2;
				}

				// Cast to a StackFirst.
				StackFirst *asFirst() {
					if (data1 & 0x1)
						// Sorry, StackFirst is not defined yet.
						return reinterpret_cast<StackFirst *>(this);
					else
						return null;
				}

				// Cast to a StackRule.
				StackRule *asRule() {
					if (data1 & 0x2)
						// Sorry, StackRule is not defined yet.
						return reinterpret_cast<StackRule *>(this);
					else
						return null;
				}

				// Get the tree here, if any.
				GcArray<TreePart> *match() const;

				// Get our depth.
				Nat depth() const {
					return myDepth;
				}

				// Compare items for the priority queue.
				Bool operator <(const StackItem &o) const {
					if (inputPos() != o.inputPos())
						return inputPos() < o.inputPos();
					else
						return myDepth < o.myDepth;
				}

			protected:
				// Set this class as a StackFirst.
				void setFirst() {
					data1 |= 0x1;
				}

				// Set this class as a StackRule.
				void setRule() {
					data1 |= 0x2;
				}

				// Depth of this node. Approximately, how deep the tree is. Used for ordering the priority queue.
				// TODO: When the deduplication is working properly, we might not need this anymore.
				Nat myDepth;

			private:
				// Data. Stores "inputPos" in topmost 30 bits, then if we're a StackRule, then if we're a StackFirst.
				Nat data1;
			};


			/**
			 * An item that represents a nonterminal match.
			 *
			 * The nonterminal "prev->iter.token()" was matched, and the result is contained in "match".
			 */
			class StackRule : public StackItem {
				STORM_CLASS;
			public:
				// Create.
				StackRule(MAYBE(StackItem *) prev, ProductionIter iter, Nat inputPos, GcArray<TreePart> *match)
					: StackItem(prev, iter, inputPos), match(match) {
					setRule();
				}

				// The match.
				GcArray<TreePart> *match;
			};


			/**
			 * An item that is the first item in a sequence.
			 *
			 * This one stores some additional information. Among others, what this entire
			 * production expanded to (for a given ending position), and possibly multiple previous
			 * states.
			 */
			class StackFirst : public StackItem {
				STORM_CLASS;
			public:
				// Create.
				StackFirst(MAYBE(StackItem *) prev, ProductionIter iter, Nat inputPos)
					: StackItem(prev, iter, inputPos) {
					setFirst();
					if (prev)
						myDepth = prev->depth() + 1;
				}

				// The current longest location this sequence matched.
				Nat matchEnd;

				// The current match, if any.
				GcArray<TreePart> *match;

				// Number of nodes here.
				Nat prevCount() const {
					if (morePrev)
						return morePrev->filled + 1;
					else
						return 1;
				}

				// Get node with index.
				// Note that we might return 'null' for index 0 to indicate the start production.
				StackItem *prevAt(Nat id) const {
					if (id == 0)
						return prev;
					else
						return morePrev->v[id];
				}

				// Add another previous node.
				void prevPush(StackItem *item) {
					// If 'prev' was null, then this is the start production. We need to remember that.

					if (morePrev == null) {
						morePrev = runtime::allocArray<StackItem *>(engine(), &pointerArrayType, 10);
					} else if (morePrev->filled >= morePrev->count) {
						GcArray<StackItem *> *n =
							runtime::allocArray<StackItem *>(engine(), &pointerArrayType, morePrev->count * 2);
						memcpy(n->v, morePrev->v, sizeof(StackItem *) * morePrev->count);
						n->filled = morePrev->filled;
						morePrev = n;
					}

					morePrev->v[morePrev->filled++] = item;
				}

			private:
				// More previous nodes. This is in addition to 'prev' in the base class. This might
				// be null if it is not needed.
				GcArray<StackItem *> *morePrev;
			};


			inline GcArray<TreePart> *StackItem::match() const {
				if (data1 & 0x2)
					// Sorry, StackRule is not defined yet.
					return reinterpret_cast<const StackRule *>(this)->match;
				else
					return null;
			}


		}
	}
}
