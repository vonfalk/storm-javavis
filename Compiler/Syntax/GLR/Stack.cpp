#include "stdafx.h"
#include "Stack.h"
#include "Core/StrBuf.h"
#include "Utils/Bitwise.h"

namespace storm {
	namespace syntax {
		namespace glr {

			const Nat StackItem::EMPTY = -1;

			StackItem::StackItem()
				: state(0), pos(0), prev(EMPTY), morePrev(EMPTY), tree(null) {}

			StackItem::StackItem(Nat state, Nat pos)
				: state(state), pos(pos), prev(EMPTY), morePrev(EMPTY), tree(null) {}

			StackItem::StackItem(Nat state, Nat pos, Nat prev, TreeNode *tree)
				: state(state), pos(pos), prev(prev), morePrev(EMPTY), tree(tree) {}

			void StackItem::updateTree(TreeNode *newTree, Syntax *syntax) {
				if (!newTree) {
				} else if (!tree) {
					// Note: this should really update any previous tree nodes, but as there is no
					// previous, we can not do that. Only the start node will ever be empty, so this
					// is not a problem.
					tree = newTree;
				} else if (newTree->priority(tree, syntax) == TreeNode::higher) {
					// Note: we can not simply set the tree pointer of this state, as we need to
					// update any previously created syntax trees.
					tree->pos = newTree->pos;
					tree->children = newTree->children;
				}
			}


			/**
			 * Storage of stacks.
			 */

			StackStore::StackStore() : count(0) {
				chunks = runtime::allocArray<Chunk *>(engine(), &pointerArrayType, 16);
				arrayType = StormInfo<StackItem>::handle(engine()).gcArrayType;
			}

			Nat StackStore::push(const StackItem &item) {
				Nat id = chunkId(count);
				if (id >= chunks->count)
					grow();

				Chunk *&chunk = chunks->v[id];
				if (!chunk)
					chunk = runtime::allocArray<StackItem>(engine(), arrayType, chunkSize);

				chunk->v[chunkOffset(count)] = item;
				return count++;
			}

			void StackStore::grow() {
				GcArray<Chunk *> *n = runtime::allocArray<Chunk *>(engine(), &pointerArrayType, chunks->count * 2);
				memcpy(n->v, chunks->v, sizeof(Chunk *)*chunks->count);
				chunks = n;
			}

			Nat StackStore::insert(Syntax *syntax, const StackItem &item, Nat into) {
				Nat last = into;
				for (Nat now = into; now != StackItem::EMPTY; now = at(now).prev) {
					last = now;

					StackItem &z = at(now);
					if (z.prev == item.prev) {
						// These are considered to be the same link. See which syntax tree to use!
						z.updateTree(item.tree, syntax);
						return StackItem::EMPTY;
					}
				}

				Nat id = push(item);
				at(last).morePrev = id;
				return id;
			}

			StackPtr::StackPtr(Nat state, Nat id) : state(state), id(id) {}

			/**
			 * Future stacks.
			 */

			FutureStacks::FutureStacks(StackStore *store) : store(store) {
				data = null;
			}

			MAYBE(Set<StackPtr> *) FutureStacks::top() {
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

			void FutureStacks::put(Nat pos, Syntax *syntax, const StackItem &insert) {
				if (pos >= count())
					grow(pos + 1);

				Nat i = wrap(first + pos);
				Set<StackPtr> *&to = data->v[i];
				if (!to)
					to = new (this) Set<StackPtr>();


				StackPtr &old = to->at(StackPtr(insert.state, StackItem::EMPTY));
				if (old.id == StackItem::EMPTY) {
					// Insertion was performed. Nothing more to do.
					old.id = store->push(insert);
				} else {
					// Append the current node as an alternative.
					store->insert(syntax, insert, old.id);
				}
			}

			void FutureStacks::grow(Nat cap) {
				cap = max(Nat(32), nextPowerOfTwo(cap));

				GcArray<Set<StackPtr> *> *n = runtime::allocArray<Set<StackPtr> *>(engine(), &pointerArrayType, cap);
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
