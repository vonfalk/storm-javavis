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

			/**
			 * Stack items in the GLR parser.
			 *
			 * These closely resemble what the paper describes as 'links'. One node consists of one
			 * instance of this class. All instances also describe a 'link', and multiple instances
			 * can be chained together to represent all links.
			 *
			 * TODO: It could be useful to merge these into larger chunks and use one-dimensional
			 * ID:s for pointers to them.
			 */
			class StackItem : public Object {
				STORM_CLASS;
			public:
				// Create.
				STORM_CTOR StackItem();
				STORM_CTOR StackItem(Nat state, Nat pos);
				STORM_CTOR StackItem(Nat state, Nat pos, StackItem *prev, TreeNode *tree);

				// State at this point in the stack.
				Nat state;

				// Position in the input. TODO: Remove?
				Nat pos;

				// Previous item in the stack.
				MAYBE(StackItem *) prev;

				// Part of the syntax tree for this node.
				MAYBE(TreeNode *) tree;

				// More previous states? Forms a linked list of multiple StackItem nodes at the same
				// level (we ignore 'state' and 'reduced' there) of more previous items.
				MAYBE(StackItem *) morePrev;

				// Insert a node in the 'morePrev' chain if it is not already there. Returns 'true' if inserted.
				Bool STORM_FN insert(Syntax *syntax, StackItem *item);

				// Equality check and hashing.
				virtual Bool STORM_FN equals(Object *o) const;
				virtual Nat STORM_FN hash() const;

				// Output.
				virtual void STORM_FN toS(StrBuf *to) const;

			private:
				// Update 'tree' in this object if the provided tree has a higher priority than this.
				void updateTree(TreeNode *newTree, Syntax *syntax);
			};


			/**
			 * Set of future stack sets. Implements a sequence of sets where lookup is constant time
			 * and popping the front is also constant time. Any access extends the structure to the
			 * desired length.
			 */
			class FutureStacks : public ObjectOn<Compiler> {
				STORM_CLASS;
			public:
				STORM_CTOR FutureStacks();

				// Get the topmost set.
				MAYBE(Set<StackItem *> *) STORM_FN top();

				// Pop the topmost set, shifting all other indices one step.
				void STORM_FN pop();

				// Insert an item at location 'pos'. The top is at location 0.
				void STORM_FN put(Nat pos, Syntax *syntax, StackItem *item);

			private:
				// Storage. Used as a circular queue. Size is always a power of two.
				GcArray<Set<StackItem *> *> *data;

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