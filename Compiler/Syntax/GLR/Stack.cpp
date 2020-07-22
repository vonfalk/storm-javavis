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
				: state(state), pos(pos), prev(null), tree(0), required() {}

			StackItem::StackItem(Nat state, Nat pos, StackItem *prev, Nat tree)
				: state(state), pos(pos), prev(prev), tree(tree), required() {}

			StackItem::StackItem(Nat state, Nat pos, StackItem *prev, Nat tree, ParentReq required)
				: state(state), pos(pos), prev(prev), tree(tree), required(required) {}

			Bool StackItem::insert(TreeStore *store, StackItem *insert) {
				Bool z;
				return this->insert(store, insert, z);
			}

			Bool StackItem::insert(TreeStore *store, StackItem *insert, Bool &usedTree) {
				// First: see if this node is already here.
				for (StackItem *at = this; at; at = at->morePrev)
					if (at == insert)
						return false;

				// Find a node to merge.
				StackItem *last = this;
				for (StackItem *at = this; at; at = at->morePrev) {
					last = at;

					if (at->prev == insert->prev && at->required == insert->required) {
						// These are considered to be the same link. See which syntax tree to use!
						usedTree |= at->updateTree(store, insert->tree);
						return false;
					}
				}

				last->morePrev = insert;
				usedTree = true;
				return true;
			}

			Bool StackItem::updateTree(TreeStore *store, Nat newTree) {
				Bool used = false;
				if (!newTree) {
				} else if (!tree) {
					// Note: this should really update any previous tree nodes, but as there is no
					// previous, we can not do that. Only the start node will ever be empty, so this
					// is not a problem.
					tree = newTree;
				} else if (store->at(tree).pos() != store->at(newTree).pos()) {
					// Don't alter the position of the tree. This can happen during error
					// recovery and is not desirable since it will duplicate parts of the input
					// string in the syntax tree.
				} else if (store->priority(newTree, tree) == TreeStore::higher) {
					// Note: we can not simply set the tree pointer of this state, as we need to
					// update any previously created syntax trees.
					// Note: due to how 'priority' works, we can be sure that both this tree node
					// and the other one have children.
					if (!store->contains(newTree, tree)) {
						// Avoid creating cycles (could probably be skipped now that 'insert'
						// properly checks for duplicates).
						store->at(tree).replace(store->at(newTree));
						used = true;
					}
				}
				return used;
			}

			Bool StackItem::operator ==(const StackItem &o) const {
				if (!sameType(this, &o))
					return false;

				return state == o.state;
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

			MAYBE(Array<StackItem *> *) FutureStacks::top() {
				if (data)
					return data->v[first];
				else
					return null;
			}

			void FutureStacks::pop() {
				if (data) {
					data->v[first] = null;

					// // Try to re-use the array if possible.
					// Array<StackItem *> *v = data->v[first];
					// if (v)
					// 	while (v->any())
					// 		v->pop();
				}
				first = wrap(first + 1);
			}

			StackItem *FutureStacks::putRaw(Nat pos, StackItem *insert) {
				if (pos >= count())
					grow(pos + 1);

				Nat i = wrap(first + pos);
				Array<StackItem *> *&to = data->v[i];
				if (!to)
					to = new (this) Array<StackItem *>();

				// Note: There are often few enough states for this to be faster than a set.
				for (Nat i = 0, count = to->count(); i < count; i++) {
					StackItem *at = to->at(i);
					if (*at == *insert) {
						return at;
					}
				}

				// Insert it.
				to->push(insert);
				return insert;
			}

			Bool FutureStacks::put(Nat pos, TreeStore *store, StackItem *insert) {
				StackItem *inserted = putRaw(pos, insert);
				// Merge existing nodes?
				if (inserted != insert)
					return inserted->insert(store, insert);
				else
					return true;
			}

			void FutureStacks::set(Nat pos, Array<StackItem *> *v) {
				if (pos >= count())
					grow(pos + 1);

				Nat i = wrap(first + pos);
				data->v[i] = v;
			}

			void FutureStacks::grow(Nat cap) {
				cap = max(Nat(32), nextPowerOfTwo(cap));

				GcArray<Array<StackItem *> *> *n = runtime::allocArray<Array<StackItem *> *>(engine(), &pointerArrayType, cap);
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
