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
			class TreeNode : public Object {
				STORM_CLASS;
			public:
				// Create a node for a terminal symbol.
				STORM_CTOR TreeNode(Nat pos);

				// Create a node for a nonterminal symbol.
				STORM_CTOR TreeNode(Nat pos, Nat production, Nat children);

				// The position at which this node started in the syntax tree.
				Nat pos;

				// Children (if allocated). The 'filled' member of the array indicates which
				// production is represented.
				GcArray<TreeNode *> *children;

				// Get the reduced id if children is set.
				inline Nat production() const { return children->filled; }

				enum Priority {
					higher,
					equal,
					lower,
				};

				// Count the number of nodes in this tree.
				Nat STORM_FN countNodes() const;

				// Find out if this node has higher priority than another node.
				Priority STORM_FN priority(TreeNode *other, Syntax *syntax);

				// Does this tree contain another node?
				Bool STORM_FN contains(TreeNode *other) const;

				// Output.
				virtual void STORM_FN toS(StrBuf *to) const;

				// Hash function and equality.
				virtual Bool STORM_FN equals(Object *o) const;
				virtual Nat STORM_FN hash() const;

			private:
				typedef PODArray<TreeNode *, 40> TreeArray;

				// Traverse the node and find all subtrees. Respects the pseudoproductions. Returns
				// true if the node was traversed.
				void allChildren(TreeArray &out, Nat productionId);
				bool addMe(TreeArray &out, Nat productionId);
			};

		}
	}
}
