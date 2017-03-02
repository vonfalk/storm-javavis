#pragma once
#include "Core/Object.h"
#include "Core/GcArray.h"
#include "Core/PODArray.h"
#include "Syntax.h"

namespace storm {
	namespace syntax {
		namespace glr {
			STORM_PKG(lang.bnf.glr);

			/**
			 * Representation of the internal syntax trees built on reductions.
			 *
			 * A tree node either represents a terminal symbol or a non-terminal symbol depending on
			 * wether or not the 'children' array is allocated. If it is allocated, it represents a
			 * nonterminal.
			 */
			class TreeNode {
				STORM_VALUE;
			public:
				// Create a node for a terminal symbol.
				STORM_CTOR TreeNode(Nat pos);

				// Create a node for a nonterminal symbol.
				TreeNode(Engine &e, Nat pos, Nat production, Nat children);

				// The position at which this node started in the syntax tree.
				Nat pos;

				// Children (if allocated). The 'filled' member of the array indicates which
				// production is represented.
				GcArray<Nat> *children;

				// Get the reduced id if children is set.
				inline Nat production() const { return children->filled; }
			};


			/**
			 * Batch allocation of tree nodes. Note: Element #0 is always empty (acts as the null pointer).
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
				inline TreeNode &at(Nat i) const {
					return chunks->v[chunkId(i)]->v[chunkOffset(i)];
				}

				// Add an item and return its ID.
				Nat push(const TreeNode &item);

				// Priority for tree comparisions.
				enum Priority {
					higher,
					equal,
					lower,
				};

				// Find out if a node has a higher priority compared to another.
				Priority STORM_FN priority(Nat a, Nat b);

			private:
				enum {
					// Bits used for each chunk.
					chunkBits = 6,
					// Elements per chunk. Assumed to be a power of two.
					chunkSize = 1 << chunkBits,
				};

				// Compute the chunk id and the chunk offset.
				inline Nat chunkId(Nat id) const { return id >> chunkBits; }
				inline Nat chunkOffset(Nat id) const { return id & (chunkSize - 1); }

				// List of all chunks.
				typedef GcArray<TreeNode> Chunk;
				GcArray<Chunk *> *chunks;

				// Current number of elements.
				Nat count;

				// Remember the syntax.
				Syntax *syntax;

				// Gc type for arrays.
				const GcType *arrayType;

				// Grow 'chunks'.
				void grow();

				// Type for storing the expanded list of children.
				typedef PODArray<Nat, 40> TreeArray;

				// Traverse the node 'x' and find all subtrees wrt to pseudoproductions.
				void allChildren(TreeArray &out, Nat productionId, TreeNode &node);
				bool addNode(TreeArray &out, Nat productionId, Nat node);
			};

		}
	}
}
