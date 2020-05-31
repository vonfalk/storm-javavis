#pragma once
#include "Core/Object.h"
#include "Core/GcArray.h"
#include "Core/Set.h"
#include "Compiler/Thread.h"
#include "Compiler/Syntax/InfoErrors.h"
#include "Syntax.h"
#include "Tree.h"
#include "ParentReq.h"

namespace storm {
	namespace syntax {
		namespace glr {
			STORM_PKG(lang.bnf.glr);

			/**
			 * Action plan for optimizing this:
			 *
			 * - Skip using a Set<> for the stack. In practice there are few (<25-30) items in each set, so
			 *   we're probably fine with a plain array (perhaps we should store "state" separately for quick iteration)
			 * - Think about the ratio of used vs discarded StackItems, if many are retained, we should make it a flat
			 *   structure. Otherwise, we might benefit from keeping the objects GC:d.
			 * - Make StackItem a chunked and indexed structure, very much like the tree.
			 * - Perhaps we should do something about the ParentReq member. Currently it contains a pointer, which
			 *   could be an issue, since the GC would then have to scan entire arrays. We could probably do something
			 *   similar to the dynamic structure used in TreeStore, since we could then reduce the entire structure
			 *   to a set of integers.
			 */

			class StackStore;
			inline Nat read(StackStore *src, Nat pos);
			inline void write(StackStore *src, Nat pos, Nat val);
			inline Nat reqCount(StackStore *src);

			/**
			 * Stack items in the GLR parser.
			 *
			 * These closely resemble what the paper describes as 'links'. One node consists of one
			 * instance of this class. All instances also describe a 'link', and multiple instances
			 * can be chained together to represent all links.
			 *
			 * TODO: It could be useful to merge these into larger chunks and use one-dimensional
			 * ID:s for pointers to them, like what we do with the tree representation.
			 */
			class StackItemZ : public Object {
				STORM_CLASS;
			public:
				// Create.
				STORM_CTOR StackItemZ();
				STORM_CTOR StackItemZ(Nat state, Nat pos);
				STORM_CTOR StackItemZ(Nat state, Nat pos, StackItemZ *prev, Nat tree);
				STORM_CTOR StackItemZ(Nat state, Nat pos, StackItemZ *prev, Nat tree, ParentReq req);

				// State at this point in the stack.
				Nat state;

				// Position in the input. TODO: Can we remove this somehow?
				Nat pos;

				// Part of the syntax tree for this node.
				Nat tree;

				// Previous item in the stack.
				MAYBE(StackItemZ *) prev;

				// More previous states? Forms a linked list of multiple StackItem nodes at the same
				// level (we ignore 'state' there) of more previous items.
				MAYBE(StackItemZ *) morePrev;

				// Required parent productions for this state.
				ParentReq required;

				// Insert a node in the 'morePrev' chain if it is not already there. Returns 'true' if inserted.
				Bool STORM_FN insert(TreeStore *store, StackItemZ *item);
				Bool insert(TreeStore *store, StackItemZ *item, Bool &usedTree);

				// Equality check and hashing.
				virtual Bool STORM_FN operator ==(const StackItemZ &o) const;
				virtual Nat STORM_FN hash() const;

				// Output.
				virtual void STORM_FN toS(StrBuf *to) const;

			private:
				// Update 'tree' in this object if the provided tree has a higher priority than this.
				Bool updateTree(TreeStore *store, Nat newTree);
			};


			/**
			 * Set of future stack sets. Implements a sequence of sets where lookup is constant time
			 * and popping the front is also constant time. Any access extends the structure to the
			 * desired length.
			 */
			class FutureStacksZ : public ObjectOn<Compiler> {
				STORM_CLASS;
			public:
				STORM_CTOR FutureStacksZ();

				// Get the topmost set.
				MAYBE(Set<StackItemZ *> *) STORM_FN top();

				// Pop the topmost set, shifting all other indices one step.
				void STORM_FN pop();

				// Insert an item at location 'pos'. The top is at location 0. Returns false if
				// rejected due to a duplicate node.
				Bool STORM_FN put(Nat pos, TreeStore *store, StackItemZ *item);

				// Set the top item to some value.
				void STORM_FN set(Nat pos, Set<StackItemZ *> *v);

			private:
				// Storage. Used as a circular queue. Size is always a power of two.
				GcArray<Set<StackItemZ *> *> *data;

				// First element position.
				Nat first;

				// Grow to fit at least 'n' elements.
				void grow(Nat n);

				// Number of elements in 'data' currently.
				inline Nat count() const { return data ? data->count : 0; }

				// Wrap 'n' to the size of data.
				inline Nat wrap(Nat n) const { return data ? (n & (data->count - 1)) : 0; }
			};

			/**
			 * Parent rule requirements for a stack item.
			 *
			 * Stores a set of parent ID:s that need to be present at some future point during
			 * parsing (i.e. as a parent). Basically, a bit-array that is sized to fit the maximum
			 * required ID currently visible in the grammar.
			 *
			 * This array is stored inside the StackStore, right after a StackItem entry.
			 */
			class ItemReq {
				STORM_VALUE;
			public:
				// Create a 'null' item.
				ItemReq() { ptr = 0; store = null; }

				// Get our ID.
				inline Nat id() const { return ptr; }

				// Append elements from another ItemReq.
				void concat(ItemReq other) {
					Nat count = reqCount(store);
					for (Nat i = 0; i < count; i++) {
						write(store, ptr + i, read(store, ptr + i) | read(other.store, other.ptr + i));
					}
				}

				// Add an element from a ReqId.
				void add(ReqId id) {
					if (id.any()) {
						const Nat sz = sizeof(Nat) * CHAR_BIT;
						Nat offset = id.v() / sz;
						Nat bit = id.v() % sz;
						write(store, ptr + offset,
							read(store, ptr + offset) | (Nat(1) << bit));
					}
				}

				// Remove elements from another ItemReq.
				void remove(ItemReq other) {
					Nat count = reqCount(store);
					for (Nat i = 0; i < count; i++) {
						write(store, ptr + i, read(store, ptr + i) & ~read(other.store, other.ptr + i));
					}
				}

				// Remove an element from a ReqId.
				void remove(ReqId id) {
					if (id.any()) {
						const Nat sz = sizeof(Nat) * CHAR_BIT;
						Nat offset = id.v() / sz;
						Nat bit = id.v() % sz;
						write(store, ptr + offset,
							read(store, ptr + offset) & ~(Nat(1) << bit));
					}
				}

				// Is this entirely empty?
				Bool empty() const {
					if (ptr == 0)
						return true;

					Nat count = reqCount(store);
					for (Nat i = 0; i < count; i++) {
						if (read(store, ptr + i) != 0)
							return false;
					}

					return true;
				}
				Bool any() const {
					return !empty();
				}

				// Do we contain a particular ReqId?
				Bool has(ReqId id) {
					if (id.any()) {
						const Nat sz = sizeof(Nat) * CHAR_BIT;
						Nat offset = id.v() / sz;
						Nat bit = id.v() % sz;
						return (read(store, ptr + offset) & (Nat(1) << bit)) != 0;
					} else {
						return false;
					}
				}

				// Count number of bits set.
				Nat count() const {
					Nat result = 0;
					Nat count = reqCount(store);
					for (Nat i = 0; i < count; i++) {
						result += setBitCount(read(store, ptr + i));
					}
					return result;
				}

				// Are the two item sets equal?
				Bool operator ==(const ItemReq &other) const {
					if ((ptr == 0) != (other.ptr == 0))
						return false;

					if (ptr == 0)
						return true;

					Nat count = reqCount(store);
					for (Nat i = 0; i < count; i++) {
						if (read(store, ptr + i) != read(other.store, other.ptr + i))
							return false;
					}

					return true;
				}

			private:
				friend class StackItem;
				friend class StackStore;

				// Create. Done by StackItem.
				ItemReq(StackStore *store, Nat ptr) {
					this->store = store;
					this->ptr = ptr;
				}

				// Backing storage.
				StackStore *store;

				// Location.
				Nat ptr;
			};


			/**
			 * Stack items in the GLR parser.
			 *
			 * These closely resemble what the paper describes as 'links'. One node consists of one
			 * instance of this class. All instances also describe a 'link', and multiple instances
			 * can be chained together to represent all links.
			 *
			 * This is a "handle" to a location inside a StackStore instance.
			 *
			 * TODO: Re-use the arrays inside here as much as possible. We don't need to discard the
			 * array as soon as we shift the data, we just need to clear it.
			 */
			class StackItem {
				STORM_VALUE;
			public:
				// Create a 'null' item.
				StackItem() { ptr = 0; store = null; }

				// Is this a "null" item?
				inline Bool any() const { return ptr > 0; }

				// Get our ID.
				inline Nat id() const { return ptr; }

				// State at this point in the stack.
				inline Nat state() const { return read(store, ptr) & 0x7FFFFFFF; }
				inline void state(Nat v) { write(store, ptr, v); }

				// Do we need to keep this state?
				inline Bool keep() const { return (read(store, ptr) & 0x80000000) != 0; }
				inline void keepThis() const { write(store, ptr, 0x80000000 | read(store, ptr)); }

				// Position in the input.
				inline Nat pos() const { return read(store, ptr + 1); }

				// Our tree.
				inline Nat tree() const { return read(store, ptr + 2); }
				inline void tree(Nat v) { write(store, ptr + 2, v); }

				// Previous item in the stack.
				inline StackItem prev() const {
					return StackItem(store, read(store, ptr + 3));
				}

				// More previous states? Forms a linked list of multiple StackItem nodes at the same
				// level (we ignore 'state' there) of more previous items.
				inline StackItem morePrev() const {
					return StackItem(store, read(store, ptr + 4));
				}
				inline void morePrev(StackItem v) {
					write(store, ptr + 4, v.id());
				}

				// Required parent productions for this state.
				inline ItemReq required() const {
					return ItemReq(store, ptr + 5);
				}

				// Insert a node in the 'morePrev' chain if it is not already there. Returns 'true' if inserted.
				Bool insert(TreeStore *store, StackItem item);
				Bool insert(TreeStore *store, StackItem item, Bool &usedTree);

				// Equality check.
				inline Bool STORM_FN operator ==(const StackItem &o) const {
					return state() == o.state();
				}

			private:
				friend class StackStore;

				// Create. Done by StackStore.
				StackItem(StackStore *store, Nat ptr) {
					this->store = store;
					this->ptr = ptr;
				}

				// Backing storage.
				StackStore *store;

				// Location.
				Nat ptr;

				// Update 'tree' in this object if the provided tree has a higher priority than this.
				Bool updateTree(TreeStore *store, Nat newTree);
			};


			/**
			 * Batch allocation of stack nodes. Note: Element #0 is always empty (it is the null pointer).
			 *
			 * This is just an array of Nat elements. The classes StackItem are more or less just
			 * pointers into this large array of Nat:s.
			 *
			 * We're doing this since the GLR parser creates many of these during parsing, and
			 * having them as a linked structure took a heavy toll on the GC. The initial idea was
			 * that this would work out fairly well anyway, as many of them are short-lived. Even
			 * though this might be the case, profiling shows that a lot of time is spent in the GC
			 * during parsing. By storing data as large Nat arrays, the GC will not have to scan
			 * them. The downside is that the GC is unable to reclaim data unless we discard the
			 * entire block.
			 */
			class StackStore : public Object {
				STORM_CLASS;
			public:
				// Create, give the maximum size required by the parent ID:s at each location. This
				// defines the size required for each item.
				STORM_CTOR StackStore(Nat maxParentId);

				// Create StackItem nodes.
				StackItem createItem();
				StackItem createItem(Nat state, Nat pos);
				StackItem createItem(Nat state, Nat pos, StackItem prev, Nat tree);
				StackItem createItem(Nat state, Nat pos, StackItem prev, Nat tree, ItemReq req);
				StackItem createItem(StackItem original);

				// "Read" an item from a pointer.
				inline StackItem readItem(Nat pos) { return StackItem(this, pos); }


				// Size information.
				inline Nat STORM_FN count() const { return size; }
				inline Nat STORM_FN byteCount() const { return size*sizeof(Nat); }

				// Free a previous allocation.
				void free(Nat alloc);
				void free(StackItem item);

				/**
				 * Interface used by StackItem and ParentReq.
				 */

				inline Nat read(Nat i) const {
					return chunks->v[chunkId(i)]->v[chunkOffset(i)];
				}
				inline void write(Nat i, Nat v) const {
					chunks->v[chunkId(i)]->v[chunkOffset(i)] = v;
				}

				// Get the requirement array size.
				inline Nat requirementCount() const {
					return reqSize;
				}

			private:
				enum {
					// Bits used for each chunk.
					chunkBits = 8,
					// Elements per chunk. Assumed to be a power of two.
					chunkSize = 1 << chunkBits,
				};

				// Compute the chunk id and the chunk offset.
				inline Nat chunkId(Nat id) const { return id >> chunkBits; }
				inline Nat chunkOffset(Nat id) const { return id & (chunkSize - 1); }

				// List of all chunks.
				typedef GcArray<Nat> Chunk;
				GcArray<Chunk *> *chunks;

				// Current number of elements;
				Nat size;

				// Head of the free list (all elements are the same size). The list is linked using
				// the first element in each allocation.
				Nat freeHead;

				// Size of the requirement array for each item in this store, in words.
				Nat reqSize;

				// Allocate room for 'n' more items.
				Nat alloc(Nat n);

				// Grow 'chunks'.
				void grow();
			};

			inline Nat read(StackStore *src, Nat pos) {
				return src->read(pos);
			}

			inline void write(StackStore *src, Nat pos, Nat val) {
				src->write(pos, val);
			}

			inline Nat reqCount(StackStore *src) {
				return src->requirementCount();
			}


			/**
			 * Set of future stack sets. Implements a sequence of sets where lookup is constant time
			 * and popping the front is also constant time. Any access extends the structure to the
			 * desired length.
			 */
			class FutureStacks : public Object {
				STORM_CLASS;
			public:
				STORM_CTOR FutureStacks(StackStore *store);

				// Get the contents of the topmost set, as references into 'store'.
				MAYBE(Array<Nat> *) STORM_FN top();

				// Pop the topmost set, shifting all other indices one step.
				void STORM_FN pop();

				// Insert an item at location 'pos', without attempting to merge nodes if the state
				// is already present there. Returns either 'item' on success, or the item that is
				// already present the set at the inicated position.
				StackItem STORM_FN putRaw(Nat pos, StackItem item);

				// Insert an item at location 'pos'. The top is at location 0. Returns 'false' if
				// rejected due to a duplicate node.
				Bool STORM_FN put(Nat pos, TreeStore *store, StackItem item);

				// Like 'put', but frees 'item' if it was not inserted.
				inline Bool STORM_FN putFree(Nat pos, TreeStore *store, StackItem item) {
					if (put(pos, store, item)) {
						return true;
					} else {
						this->store->free(item.id());
						return false;
					}
				}

				// Set a particular entry.
				void STORM_FN set(Nat pos, Array<Nat> *value);

			private:
				// Stack storage.
				StackStore *store;

				// Storage. Used as a circular queue. Size is always a power of two.
				GcArray<Array<Nat> *> *data;

				// First element position.
				Nat first;

				// Grow to fit at least 'n' elements.
				void grow(Nat n);

				// Number of elements in 'data' currently.
				inline Nat count() const { return data ? data->count : 0; }

				// Wrap 'n' to the size of data.
				inline Nat wrap(Nat n) const { return data ? (n & (data->count - 1)) : 0; }
			};

		}
	}
}
