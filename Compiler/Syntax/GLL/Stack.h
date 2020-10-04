#pragma once
#include "Compiler/Syntax/Production.h"
#include "RuleInfo.h"
#include "Tree.h"

namespace storm {
	namespace syntax {
		namespace gll {
			STORM_PKG(lang.bnf.gll);

			class StackMatch;
			class StackRule;
			class StackEnd;
			class StackFirst;

			/**
			 * An item on the LL stack.
			 *
			 * This is the shared base class for all states. The class StackFirst represents the
			 * start of a production match, and keeps track of matches. There are also versions for
			 * states that will match a terminal and states that will match a rule, since they need
			 * slightly different state.
			 */
			class StackItem : public Object {
				STORM_CLASS;
			public:
				// Create.
				StackItem(MAYBE(StackItem *) prev, Nat inputPos)
					: prev(prev), data(inputPos << 2) {}

				// Previous item, if any. Note: If this is a StackFirst, there may be more of these
				// accessible through the interface in StackFirst.
				MAYBE(StackItem *) prev;

				// Position in the input.
				Nat inputPos() const {
					return data >> 2;
				}

				// Cast to a StackMatch if possible.
				StackMatch *asMatch() {
					if (data & tagMatch)
						return (StackMatch *)this;
					else
						return null;
				}

				// Cast to a StackRule if possible.
				StackRule *asRule() {
					if ((data & tagMask) == tagRule)
						return (StackRule *)this;
					else
						return null;
				}

				// Cast to a StackEnd if possible.
				StackEnd *asEnd() {
					if ((data & tagMask) == tagEnd)
						return (StackEnd *)this;
					else
						return null;
				}

				// Cast to a StackFirst if possible.
				StackFirst *asFirst() {
					if ((data & tagMask) == tagFirst)
						return (StackFirst *)this;
					else
						return null;
				}

				// Get the match from here.
				GcArray<TreePart> *match() const;

				// Compare items for the priority queue.
				Bool operator <(const StackItem &o) const {
					return inputPos() < o.inputPos();
				}

			protected:
				// Indicate that we're a StackMatch.
				void setMatch() {
					data |= tagMatch;
				}

				// Indicate that we're a StackRule. This may be called after 'setMatch' in contrast
				// to the other corresponding 'setXxx' functions.
				void setRule() {
					data |= tagRule;
				}

				// Indicate that we're a StackFirst.
				void setFirst() {
					data |= tagFirst;
				}

				// Set that we're a StackEnd.
				void setEnd() {
					data |= tagEnd;
				}

				// It is fine to use virtual functions here, it is only for debugging.
				virtual void STORM_FN toS(StrBuf *to) const {
					*to << (void *)this << S(", at ") << inputPos() << S(", prev ") << (void *)prev;
				}

			private:
				// Type tags. Note: It is important that 'tagRule' is an extension of 'tagMatch'.
				enum {
					tagEnd = 0x00,
					tagMatch = 0x01,
					tagRule = 0x03,
					tagFirst = 0x02,

					tagMask = 0x03,
				};

				// Data. Stores "inputPos" in the topmost 30 bits, then a type tag for quick downcasting.
				Nat data;
			};


			/**
			 * Item on the stack matching something. In the basic form, a regex is matched, in case
			 * of the derived class 'StackRule', then it is a rule.
			 */
			class StackMatch : public StackItem {
				STORM_CLASS;
			public:
				// Create.
				StackMatch(MAYBE(StackItem *) prev, ProductionIter iter, Nat inputPos)
					: StackItem(prev, inputPos), iter(iter) {
					setMatch();
				}

				// Current position.
				ProductionIter iter;

			protected:
				// It is fine to use virtual functions here, it is only for debugging.
				virtual void STORM_FN toS(StrBuf *to) const {
					StackItem::toS(to);
					*to << S(" - ") << iter;
				}
			};


			/**
			 * An item that represents a rule match. This means that 'iter' points to a rule currently.
			 */
			class StackRule : public StackMatch {
				STORM_CLASS;
			public:
				// Create.
				StackRule(MAYBE(StackItem *) prev, ProductionIter iter, Nat inputPos)
					: StackMatch(prev, iter, inputPos), match(null) {
					setRule();
				}

				// Current match, if any yet.
				GcArray<TreePart> *match;
			};


			/**
			 * An item that represents reaching the end of a production.
			 */
			class StackEnd : public StackItem {
				STORM_CLASS;
			public:
				// Create
				StackEnd(MAYBE(StackItem *) prev, Production *p, Nat inputPos)
					: StackItem(prev, inputPos), production(p) {
					setEnd();
				}

				// Which production was completed.
				Production *production;

			protected:
				virtual void STORM_FN toS(StrBuf *to) const {
					StackItem::toS(to);
					*to << S(" - end of ") << production;
				}
			};


			/**
			 * An item that represents the start of a new rule.
			 *
			 * This particular node may have multiple previous states, as the same rule could be
			 * matched from multiple locations in the grammar at the same offset. In such cases, we
			 * merge the matches so we don't have to waste computational resources on doing the same
			 * match multiple times, and therefore we need multiple previous states here.
			 */
			class StackFirst : public StackItem {
				STORM_CLASS;
			public:
				// Create.
				StackFirst(MAYBE(StackRule *) prev, RuleInfo *rule, Nat inputPos)
					: StackItem(prev, inputPos), rule(rule) {
					setFirst();
				}

				// The rule we are to match.
				RuleInfo *rule;

				// The current match, if any.
				GcArray<TreePart> *match;

				// End of the match, if any.
				Nat matchEnd;

				// Number of nodes here.
				Nat prevCount() const {
					if (morePrev)
						return morePrev->filled + 1;
					else if (prev)
						return 1;
					else
						return 0;
				}

				// Get node with index.
				StackRule *prevAt(Nat id) {
					if (id == 0)
						return (StackRule *)prev;
					else
						return morePrev->v[id - 1];
				}

				// Set a node with a particular index.
				void prevAt(Nat id, StackRule *update) {
					if (id == 0)
						prev = update;
					else
						morePrev->v[id - 1] = update;
				}

				// Add another previous node.
				void prevPush(StackRule *item) {
					if (!prev) {
						prev = item;
						return;
					}

					if (morePrev == null) {
						morePrev = runtime::allocArray<StackRule *>(engine(), &pointerArrayType, 10);
					} else if (morePrev->filled >= morePrev->count) {
						GcArray<StackRule *> *n =
							runtime::allocArray<StackRule *>(engine(), &pointerArrayType, morePrev->count * 2);
						memcpy(n->v, morePrev->v, sizeof(StackRule *) * morePrev->count);
						n->filled = morePrev->filled;
						morePrev = n;
					}

					morePrev->v[morePrev->filled++] = item;
				}

			protected:
				// It is fine to use virtual functions here, it is only for debugging.
				virtual void STORM_FN toS(StrBuf *to) const {
					StackItem::toS(to);
					*to << S(" - ") << rule;
				}

			private:
				// More previous nodes. This is in addition to 'prev' in the base class. This might
				// be null if it is not needed.
				GcArray<StackRule *> *morePrev;
			};


			inline GcArray<TreePart> *StackItem::match() const {
				if ((data & tagMask) == tagRule)
					return ((const StackRule *)this)->match;
				else
					return null;
			}


		}
	}
}
