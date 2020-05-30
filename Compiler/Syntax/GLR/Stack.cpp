#include "stdafx.h"
#include "Stack.h"
#include "Core/StrBuf.h"
#include "Utils/Bitwise.h"

namespace storm {
	namespace syntax {
		namespace glr {

			StackItemZ::StackItemZ()
				: state(0), pos(0), prev(null), tree(0) {}

			StackItemZ::StackItemZ(Nat state, Nat pos)
				: state(state), pos(pos), prev(null), tree(0), required() {}

			StackItemZ::StackItemZ(Nat state, Nat pos, StackItemZ *prev, Nat tree)
				: state(state), pos(pos), prev(prev), tree(tree), required() {}

			StackItemZ::StackItemZ(Nat state, Nat pos, StackItemZ *prev, Nat tree, ParentReq required)
				: state(state), pos(pos), prev(prev), tree(tree), required(required) {}

			Bool StackItemZ::insert(TreeStore *store, StackItemZ *insert) {
				Bool z;
				return this->insert(store, insert, z);
			}

			Bool StackItemZ::insert(TreeStore *store, StackItemZ *insert, Bool &usedTree) {
				// First: see if this node is already here.
				for (StackItemZ *at = this; at; at = at->morePrev)
					if (at == insert)
						return false;

				// Find a node to merge.
				StackItemZ *last = this;
				for (StackItemZ *at = this; at; at = at->morePrev) {
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

			Bool StackItemZ::updateTree(TreeStore *store, Nat newTree) {
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

			Bool StackItemZ::operator ==(const StackItemZ &o) const {
				if (!sameType(this, &o))
					return false;

				return state == o.state;
			}

			Nat StackItemZ::hash() const {
				return state;
			}

			static void print(const StackItemZ *me, const StackItemZ *end, StrBuf *to) {
				bool space = false;
				for (const StackItemZ *i = me; i; i = i->prev) {
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

			void StackItemZ::toS(StrBuf *to) const {
				print(this, null, to);
			}


			/**
			 * Future stacks.
			 */

			FutureStacksZ::FutureStacksZ() {
				data = null;
			}

			MAYBE(Set<StackItemZ *> *) FutureStacksZ::top() {
				if (data)
					return data->v[first];
				else
					return null;
			}

			void FutureStacksZ::pop() {
				if (data)
					data->v[first] = null;
				first = wrap(first + 1);
			}

			Bool FutureStacksZ::put(Nat pos, TreeStore *store, StackItemZ *insert) {
				if (pos >= count())
					grow(pos + 1);

				Nat i = wrap(first + pos);
				Set<StackItemZ *> *&to = data->v[i];
				if (!to)
					to = new (this) Set<StackItemZ *>();


				StackItemZ *old = to->at(insert);
				if (old == insert) {
					// Insertion was performed. Nothing more to do.
					return true;
				} else {
					// Append the current node as an alternative.
					return old->insert(store, insert);
				}
			}

			void FutureStacksZ::set(Nat pos, Set<StackItemZ *> *v) {
				if (pos >= count())
					grow(pos + 1);

				Nat i = wrap(first + pos);
				data->v[i] = v;
			}

			void FutureStacksZ::grow(Nat cap) {
				cap = max(Nat(32), nextPowerOfTwo(cap));

				GcArray<Set<StackItemZ *> *> *n = runtime::allocArray<Set<StackItemZ *> *>(engine(), &pointerArrayType, cap);
				Nat c = count();
				for (Nat i = 0; i < c; i++) {
					n->v[i] = data->v[wrap(first + i)];
				}

				data = n;
				first = 0;
			}


			/**
			 * Stack item.
			 */

			Bool StackItem::insert(TreeStore *store, StackItem insert) {
				Bool z = false;
				return this->insert(store, insert, z);
			}

			Bool StackItem::insert(TreeStore *store, StackItem insert, Bool &usedTree) {
				// First: see if this node is already here.
				for (StackItem at = *this; at.any(); at = at.morePrev())
					if (at.id() == insert.id())
						return false;

				// Find a node to merge.
				StackItem last = *this;
				for (StackItem at = *this; at.any(); at = at.morePrev()) {
					last = at;

					if (at.prev().id() == insert.prev().id() && at.required() == insert.required()) {
						// These are considered to be the same link. See which syntax tree to use!
						usedTree |= at.updateTree(store, insert.tree());
						return false;
					}
				}

				write(last.store, last.ptr + 4, insert.id()); // Set 'morePrev' of 'last'.
				usedTree = true;
				return true;
			}

			Bool StackItem::updateTree(TreeStore *store, Nat newTree) {
				Bool used = false;
				Nat ourTree = tree();
				if (!newTree) {
				} else if (!ourTree) {
					// Note: this should really update any previous tree nodes, but as there is no
					// previous, we can not do that. Only the start node will ever be empty, so this
					// is not a problem.
					used = true;
					write(this->store, ptr + 2, newTree); // Set 'tree'.
				} else if (store->at(ourTree).pos() != store->at(newTree).pos()) {
					// Don't alter the position of the tree. This can happen during error recovery
					// and is not desirable since it will duplicate parts of the input string in the
					// syntax tree.
				} else if (store->priority(newTree, ourTree) == TreeStore::higher) {
					// Note: we can not simply set the tree pointer of this state, as we need to
					// update any previously created syntax trees.
					// Note: due to how 'priority' works, we can be sure that both this tree node
					// and the other one have children.
					if (!store->contains(newTree, ourTree)) {
						// Avoid creating cycles (could probably be skipped now that 'insert'
						// properly checks for duplicates).
						store->at(ourTree).replace(store->at(newTree));
						used = true;
					}
				}
				return used;
			}


			/**
			 * Stack storage.
			 */

			StackStore::StackStore(Nat maxParentId) : size(1), freeHead(0) {
				chunks = runtime::allocArray<Chunk *>(engine(), &pointerArrayType, 16);

				const Nat bits = sizeof(Nat) * CHAR_BIT;
				reqSize = (maxParentId + bits - 1) / bits;
			}

			StackItem StackStore::createItem() {
				return createItem(0, 0, StackItem(this, 0), 0);
			}

			StackItem StackStore::createItem(Nat state, Nat pos) {
				return createItem(state, pos, StackItem(this, 0), 0);
			}

			StackItem StackStore::createItem(Nat state, Nat pos, StackItem prev, Nat tree) {
				Nat mem = alloc(5 + reqSize);
				write(mem + 0, state);
				write(mem + 1, pos);
				write(mem + 2, tree);
				write(mem + 3, prev.id());
				write(mem + 4, 0);

				// Zero the parent set.
				for (Nat i = 0; i < reqSize; i++)
					write(mem + i + 5, 0);

				// PLN(L"Created item " << state << L" at " << mem);
				return StackItem(this, mem);
			}

			StackItem StackStore::createItem(Nat state, Nat pos, StackItem prev, Nat tree, ItemReq req) {
				Nat mem = alloc(5 + reqSize);
				write(mem + 0, state);
				write(mem + 1, pos);
				write(mem + 2, tree);
				write(mem + 3, prev.id());
				write(mem + 4, 0);

				// Copy the parent set.
				for (Nat i = 0; i < reqSize; i++)
					write(mem + i + 5, read(req.id() + i));

				return StackItem(this, mem);
			}

			StackItem StackStore::createItem(StackItem src) {
				return createItem(src.state(), src.pos(), src.prev(), src.tree(), src.required());
			}


			Nat StackStore::alloc(Nat n) {
				// Something to re-use from the free list?
				if (freeHead) {
					Nat result = freeHead;
					freeHead = read(freeHead);
					return result;
				} else {
					// Allocate new space at the end of the array.
					Nat first = chunkId(size);
					Nat last = chunkId(size + n - 1);
					while (last >= chunks->count)
						grow();

					for (Nat i = first; i <= last; i++)
						if (!chunks->v[i])
							chunks->v[i] = runtime::allocArray<Nat>(engine(), &natArrayType, chunkSize);

					Nat result = size;
					size += n;
					return result;
				}
			}

			void StackStore::free(Nat alloc) {
				// Link it in the free list.
				write(alloc, freeHead);
				freeHead = alloc;
			}

			void StackStore::free(StackItem item) {
				free(item.id());
			}

			void StackStore::grow() {
				GcArray<Chunk *> *n = runtime::allocArray<Chunk *>(engine(), &pointerArrayType, chunks->count * 2);
				memcpy(n->v, chunks->v, sizeof(Chunk *)*chunks->count);
				chunks = n;
			}

			/**
			 * Future stack sets.
			 */

			FutureStacks::FutureStacks(StackStore *store) : data(null), store(store) {}

			MAYBE(Array<Nat> *) FutureStacks::top() {
				if (data)
					return data->v[first];
				else
					return null;
			}

			void FutureStacks::pop() {
				if (data) {
					Array<Nat> *v = data->v[first];
					if (v)
						// Try to re-use the contents of the array if possible.
						while (v->any())
							v->pop();
				}
				first = wrap(first + 1);
			}

			StackItem FutureStacks::putRaw(Nat pos, StackItem insert) {
				if (pos >= count())
					grow(pos + 1);

				Array<Nat> *&to = data->v[wrap(first + pos)];
				if (!to)
					to = new (this) Array<Nat>();

				for (Nat i = 0; i < to->count(); i++) {
					StackItem item = store->readItem(to->at(i));
					if (item == insert)
						return item;
				}

				to->push(insert.id());
				return insert;
			}

			Bool FutureStacks::put(Nat pos, TreeStore *tStore, StackItem insert) {
				if (pos >= count())
					grow(pos + 1);

				Array<Nat> *&to = data->v[wrap(first + pos)];
				if (!to)
					to = new (this) Array<Nat>();

				// Note: In practice we have a maximum of 20-30 nodes, so a linear search should be
				// faster than a set with hashing etc.
				for (Nat i = 0; i < to->count(); i++) {
					StackItem item = store->readItem(to->at(i));
					if (item == insert) {
						// Append to the existing node and see if that works.
						return item.insert(tStore, insert);
					}
				}

				// Nothing found. Add a new node.
				to->push(insert.id());
				return true;
			}

			void FutureStacks::set(Nat pos, Array<Nat> *v) {
				if (pos >= count())
					grow(pos + 1);

				data->v[wrap(first + pos)] = v;
			}

			void FutureStacks::grow(Nat cap) {
				cap = max(Nat(32), nextPowerOfTwo(cap));

				GcArray<Array<Nat> *> *n = runtime::allocArray<Array<Nat> *>(engine(), &pointerArrayType, cap);
				Nat c = count();
				for (Nat i = 0; i < c; i++)
					n->v[i] = data->v[wrap(first + i)];

				data = n;
				first = 0;
			}

		}
	}
}
