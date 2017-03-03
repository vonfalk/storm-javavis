#pragma once
#include "Core/Object.h"
#include "Core/GcArray.h"
#include "Core/PODArray.h"
#include "Syntax.h"

namespace storm {
	namespace syntax {
		namespace glr {
			STORM_PKG(lang.bnf.glr);

			class TreeStore;
			extern const Nat countMask;

			inline Nat read(TreeStore *src, Nat pos);
			inline void write(TreeStore *src, Nat pos, Nat val);

			/**
			 * Easy acces to an array of children from a TreeNode.
			 */
			class TreeArray {
				STORM_VALUE;
			public:
				// Does this array exist?
				inline Bool STORM_FN exists() const { return ptr != 0; }
				inline operator bool() const { return ptr != 0; }

				// Get number of elements.
				inline Nat STORM_FN count() const {
					return cnt;
				}

				// Get element #n.
				inline Nat STORM_FN operator[] (Nat i) const {
					return read(src, ptr + i + 2);
				}

				// Set element #n.
				inline void STORM_FN set(Nat i, Nat v) const {
					write(src, ptr + i + 2, v);
				}

				// Get the reduced production.
				inline Nat STORM_FN production() const {
					return read(src, ptr + 1);
				}

			private:
				friend class TreeNode;
				inline TreeArray(TreeStore *in, Nat pos) {
					src = in;
					ptr = pos;
					cnt = read(src, ptr) & ~countMask;
				}

				// Access to data. Might be null.
				TreeStore *src;

				// Starting point of this node.
				Nat ptr;

				// Cached element count.
				Nat cnt;
			};


			/**
			 * Easy access to a tree node stored in TreeStore somewhere.
			 */
			class TreeNode {
				STORM_VALUE;
			public:
				// Get a 'pointer' to this node.
				inline Nat STORM_FN id() const {
					return ptr;
				}

				// Get the starting position of this node.
				inline Nat STORM_FN pos() const {
					return read(src, ptr) & ~countMask;
				}

				// Set the starting position of this node.
				inline void STORM_FN pos(Nat v) const {
					v &= ~countMask;
					v |= read(src, ptr) & countMask;
					return write(src, ptr, v);
				}

				// Get the child array.
				inline TreeArray STORM_FN children() const {
					if ((read(src, ptr) & countMask) == 0)
						return TreeArray(src, 0);

					Nat pos = read(src, ptr + 1);
					if (pos & countMask) {
						return TreeArray(src, ptr + 1);
					} else {
						return TreeArray(src, pos);
					}
				}

				// Replace the contents of this node with another. This is only doable if both this
				// and the other tree node have children.
				void STORM_FN replace(const TreeNode &other);

				// Get the production.
				inline Nat STORM_FN production() const {
					return children().production();
				}

			private:
				friend class TreeStore;

				// Create with a 'pointer' to the first entry used by this node.
				inline TreeNode(TreeStore *in, Nat pos) {
					src = in;
					ptr = pos;
				}

				// Access to data.
				TreeStore *src;

				// Starting point of this node.
				Nat ptr;
			};


			/**
			 * Batch allocation of tree nodes. Note: Element #0 is always empty (acts as the null pointer).
			 *
			 * This is just an array of Nat elements. The classes TreeNode and TreeArray just read
			 * from here. Nodes are stored as follows (one entry per item):
			 * - position this node started in the input string. If msb of the position is set, this node
			 *   contains children.
			 * - number of children / 'pointer' to the data array. If msb is set, then this is a count and the
			 *   child array is allocated right here. Otherwise, this is a pointer (possibly to null, which
			 *   equals no children) to the child array, starting with a number of children.
			 * - production reduced
			 * - pointer to child n (n times).
			 *
			 * Implements an array-like data structure where index lookup is constant time and
			 * growing is also (near)constant time.
			 */
			class TreeStore : public ObjectOn<Compiler> {
				STORM_CLASS;
			public:
				// Create.
				STORM_CTOR TreeStore(Syntax *syntax);

				// Item access.
				inline TreeNode at(Nat i) {
					return TreeNode(this, i);
				}

				// Add a new terminal node and return its ID.
				TreeNode push(Nat pos);

				// Add a new nonterminal with a specific number of children.
				TreeNode push(Nat pos, Nat production, Nat children);

				// Priority for tree comparisions.
				enum Priority {
					higher,
					equal,
					lower,
				};

				// Find out if a node has a higher priority compared to another.
				Priority STORM_FN priority(Nat a, Nat b);

				/**
				 * Interface that TreeNode and TreeArray are supposed to use.
				 */

				// Low-level access.
				inline Nat read(Nat i) const {
					return chunks->v[chunkId(i)]->v[chunkOffset(i)];
				}

				inline void write(Nat i, Nat v) const {
					chunks->v[chunkId(i)]->v[chunkOffset(i)] = v;
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

				// Current number of elements.
				Nat count;

				// Remember the syntax.
				Syntax *syntax;

				// Allocate room for 'n' more items.
				Nat alloc(Nat n);

				// Grow 'chunks'.
				void grow();

				// Type for storing the expanded list of children.
				typedef PODArray<Nat, 40> ChildArray;

				// Traverse the node 'x' and find all subtrees wrt to pseudoproductions.
				void allChildren(ChildArray &out, Nat productionId, TreeNode &node);
				bool addNode(ChildArray &out, Nat productionId, Nat node);
			};

			inline Nat read(TreeStore *src, Nat pos) {
				return src->read(pos);
			}

			inline void write(TreeStore *src, Nat pos, Nat val) {
				src->write(pos, val);
			}

		}
	}
}
