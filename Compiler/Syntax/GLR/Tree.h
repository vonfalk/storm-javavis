#pragma once
#include "Core/Object.h"
#include "Core/GcArray.h"
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

				// Find out if this node has higher priority than another node.
				Priority STORM_FN priority(TreeNode *other, Syntax *syntax) const;

				// Output.
				virtual void STORM_FN toS(StrBuf *to) const;
			};

		}
	}
}
