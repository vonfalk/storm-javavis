#pragma once
#include "Core/Object.h"
#include "Core/GcArray.h"
#include "Core/Set.h"
#include "Compiler/Thread.h"
#include "Syntax.h"
#include "Tree.h"

namespace storm {
	namespace syntax {
		namespace glr {
			STORM_PKG(lang.bnf.glr);

			class StackStore;

			/**
			 * Stack items in the GLR parser.
			 *
			 * These closely resemble what the paper describes as 'links'. One node consists of one
			 * instance of this class. All instances also describe a 'link', and multiple instances
			 * can be chained together to represent all links.
			 */
			class StackItem {
				STORM_VALUE;
			public:
				// Create.
				STORM_CTOR StackItem();
				STORM_CTOR StackItem(Nat state, Nat pos);
				STORM_CTOR StackItem(Nat state, Nat pos, Nat prev, TreeNode *tree);

				// Used as 'empty'.
				static const Nat EMPTY;

				// State at this point in the stack.
				Nat state;

				// Position in the input. TODO: Remove?
				Nat pos;

				// Previous item in the stack.
				Nat prev;

				// More previous states? Forms a linked list of multiple StackItem nodes at the same
				// level (we ignore 'state' there) of more previous items.
				Nat morePrev;

				// Part of the syntax tree for this node.
				MAYBE(TreeNode *) tree;

				// Update 'tree' in this object if the provided tree has a higher priority than this.
				void updateTree(TreeNode *newTree, Syntax *syntax);
			};

			/**
			 * Batch allocation of stack items.
			 *
			 * Implements an array-like data structure where index lookup is constant time and
			 * growing is also (near)constant time.
			 */
			class StackStore : public ObjectOn<Compiler> {
				STORM_CLASS;
			public:
				// Create.
				STORM_CTOR StackStore();

				// Item access.
				inline StackItem &at(Nat i) const {
					return chunks->v[chunkId(i)]->v[chunkOffset(i)];
				}

				// Add an item and return its ID.
				Nat push(const StackItem &item);

				// Insert a node into the chain of an already existing node.
				Nat insert(Syntax *syntax, const StackItem &item, Nat into);

			private:
				enum {
					// Bits used for each chunk.
					chunkBits = 5,
					// Elements per chunk. Assumed to be a power of two.
					chunkSize = 1 << chunkBits,
				};

				// Compute the chunk id and the chunk offset.
				inline Nat chunkId(Nat id) const { return id >> chunkBits; }
				inline Nat chunkOffset(Nat id) const { return id & (chunkSize - 1); }

				// List of all chunks.
				typedef GcArray<StackItem> Chunk;
				GcArray<Chunk *> *chunks;

				// Current number of elements.
				Nat count;

				// Gc type for arrays.
				const GcType *arrayType;

				// Grow 'chunks'.
				void grow();
			};


			/**
			 * Keys in a set of states. Knows the state stored in the item so that they can be
			 * properly disambiguated.
			 */
			class StackPtr {
				STORM_VALUE;
			public:
				STORM_CTOR StackPtr(Nat state, Nat id);

				Nat state;
				Nat id;

				inline Bool STORM_FN operator ==(const StackPtr &o) const {
					return state == o.state;
				}

				inline Bool STORM_FN operator !=(const StackPtr &o) const {
					return !(*this == o);
				}

				inline Nat STORM_FN hash() const {
					return state;
				}
			};


			/**
			 * Set of future stack sets. Implements a sequence of sets where lookup is constant time
			 * and popping the front is also constant time. Any access extends the structure to the
			 * desired length.
			 */
			class FutureStacks : public ObjectOn<Compiler> {
				STORM_CLASS;
			public:
				STORM_CTOR FutureStacks(StackStore *store);

				// Get the topmost set.
				MAYBE(Set<StackPtr> *) STORM_FN top();

				// Pop the topmost set, shifting all other indices one step.
				void STORM_FN pop();

				// Insert an item at location 'pos'. The top is at location 0.
				void STORM_FN put(Nat pos, Syntax *syntax, const StackItem &item);

			private:
				// Storage. Used as a circular queue. Size is always a power of two.
				GcArray<Set<StackPtr> *> *data;

				// First element position.
				Nat first;

				// Storage.
				StackStore *store;

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
