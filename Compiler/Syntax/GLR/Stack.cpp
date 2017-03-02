#include "stdafx.h"
#include "Stack.h"
#include "Core/StrBuf.h"
#include "Utils/Bitwise.h"

namespace storm {
	namespace syntax {
		namespace glr {

			StackItem::StackItem()
				: state(0), pos(0), prev(null), tree(0) {}

			StackItem::StackItem(Nat state, Nat pos)
				: state(state), pos(pos), prev(null), tree(0) {}

			StackItem::StackItem(Nat state, Nat pos, StackItem *prev, Nat tree)
				: state(state), pos(pos), prev(prev), tree(tree) {}

			Bool StackItem::insert(TreeStore *store, StackItem *insert) {
				if (insert == this)
					return false;

				StackItem *last = this;
				for (StackItem *at = this; at; at = at->morePrev) {
					last = at;

					if (at == insert)
						return false;
					if (at->prev == insert->prev) {
						// These are considered to be the same link. See which syntax tree to use!
						at->updateTree(store, insert->tree);
						return false;
					}
				}

				last->morePrev = insert;
				return true;
			}

			void StackItem::updateTree(TreeStore *store, Nat newTree) {
				if (!newTree) {
				} else if (!tree) {
					// Note: this should really update any previous tree nodes, but as there is no
					// previous, we can not do that. Only the start node will ever be empty, so this
					// is not a problem.
					tree = newTree;
				} else if (store->priority(newTree, tree) == TreeStore::higher) {
					// Note: we can not simply set the tree pointer of this state, as we need to
					// update any previously created syntax trees.
					store->at(tree) = store->at(newTree);
				}
			}

			Bool StackItem::equals(Object *o) const {
				if (runtime::typeOf(this) != runtime::typeOf(o))
					return false;

				return state == ((StackItem *)o)->state;
			}

			Nat StackItem::hash() const {
				return state;
			}

			static void print(const StackItem *me, const StackItem *end, StrBuf *to) {
				bool space = false;
				for (const StackItem *i = me; i; i = i->prev) {
					if (i == end) {
						*to << L"...";
						break;
					}
					if (space)
						*to << L" ";
					space = true;
					*to << i->state;

					if (i->morePrev)
						*to << L"->";

					if (i->tree) {
						*to << L" " << i->tree << L"\n";
						space = false;
					}
				}
			}

			void StackItem::toS(StrBuf *to) const {
				print(this, null, to);
			}


			/**
			 * Future stacks.
			 */

			FutureStacks::FutureStacks() {
				data = null;
			}

			MAYBE(Set<StackItem *> *) FutureStacks::top() {
				if (data)
					return data->v[first];
				else
					return null;
			}

			void FutureStacks::pop() {
				if (data)
					data->v[first] = null;
				first = wrap(first + 1);
			}

			void FutureStacks::put(Nat pos, TreeStore *store, StackItem *insert) {
				if (pos >= count())
					grow(pos + 1);

				Nat i = wrap(first + pos);
				Set<StackItem *> *&to = data->v[i];
				if (!to)
					to = new (this) Set<StackItem *>();


				StackItem *old = to->at(insert);
				if (old == insert) {
					// Insertion was performed. Nothing more to do.
				} else {
					// Append the current node as an alternative.
					old->insert(store, insert);
				}
			}

			void FutureStacks::grow(Nat cap) {
				cap = max(Nat(32), nextPowerOfTwo(cap));

				GcArray<Set<StackItem *> *> *n = runtime::allocArray<Set<StackItem *> *>(engine(), &pointerArrayType, cap);
				Nat c = count();
				for (Nat i = 0; i < c; i++) {
					n->v[i] = data->v[wrap(first + i)];
				}

				data = n;
				first = 0;
			}

		}
	}
}
