#include "stdafx.h"
#include "Tree.h"
#include "Core/StrBuf.h"

namespace storm {
	namespace syntax {
		namespace glr {

			TreeNode::TreeNode(Nat pos) : pos(pos), children(null) {}

			TreeNode::TreeNode(Nat pos, Nat production, Nat count) : pos(pos) {
				children = runtime::allocArray<TreeNode *>(engine(), &pointerArrayType, count);
				children->filled = production;
			}

			void TreeNode::toS(StrBuf *to) const {
				*to << L"@" << pos;

				if (children) {
					*to << L" reduced by " << children->filled << L" {\n";
					{
						Indent z(to);
						for (Nat i = 0; i < children->count; i++) {
							children->v[i]->toS(to);
							*to << L"\n";
						}
					}
					*to << L"}";
				}
			}

			TreeNode::Priority TreeNode::priority(TreeNode *b, Syntax *syntax) {
				TreeNode *a = this;
				if (!a->children || !b->children)
					return equal;
				if (a->pos != b->pos)
					return a->pos < b->pos ? higher : lower;

				Production *aProd = syntax->production(a->production());
				Production *bProd = syntax->production(b->production());
				if (aProd->priority != bProd->priority)
					return aProd->priority > bProd->priority ? higher : lower;

				if (aProd != bProd)
					// If two productions have the same priority, the behavior is undefined.
					return equal;

				// Traverse and do a lexiographic compare between the two trees.
				TreeArray aChildren(engine());
				allChildren(aChildren, a->production());
				TreeArray bChildren(engine());
				allChildren(bChildren, b->production());

				Nat to = min(aChildren.count(), bChildren.count());
				Priority result = equal;
				for (Nat i = 0; i < to && result == equal; i++) {
					result = aChildren[i]->priority(bChildren[i], syntax);
				}

				// The longest one wins. This makes * and + greedy.
				if (aChildren.count() != bChildren.count())
					return aChildren.count() > bChildren.count() ? higher : lower;

				// Nothing more to compare, they are equal to us.
				return equal;
			}

			bool TreeNode::allChildren(TreeArray &out, Nat productionId) {
				if (!children)
					return false;
				if (Syntax::baseProd(production()) != Syntax::baseProd(productionId))
					return false;

				// TODO? Make this iterative in some cases, can be done like in Parser::subtree.
				for (Nat i = 0; i < children->count; i++) {
					if (!children->v[i]->allChildren(out, productionId))
						out.push(children->v[i]);
				}

				return true;
			}

		}
	}
}
