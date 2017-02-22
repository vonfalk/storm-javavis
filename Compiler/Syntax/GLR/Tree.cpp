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
				*to << hex(this) << L"@" << pos;

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

			Bool TreeNode::equals(Object *other) const {
				if (runtime::typeOf(this) != runtime::typeOf(other))
					return false;

				TreeNode *o = (TreeNode *)other;
				bool hasChildren = children != null;
				bool oChildren = o->children != null;

				if (hasChildren != oChildren)
					return false;

				if (hasChildren)
					return pos == o->pos
						&& production() == o->production();
				else
					return pos == o->pos;
			}

			Nat TreeNode::hash() const {
				if (children)
					return pos ^ production();
				else
					return pos;
			}

			TreeNode::Priority TreeNode::priority(TreeNode *b, Syntax *syntax) {
				TreeNode *a = this;
				if (a == b)
					return equal;
				if (!a->children || !b->children)
					return equal;
				// These productions are introduced in order to fix epsilon regexes. Do not compare!
				if (Syntax::specialProd(a->production()) == Syntax::prodESkip
					|| Syntax::specialProd(a->production()) == Syntax::prodESkip)
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
				a->allChildren(aChildren, Syntax::baseProd(a->production()));
				TreeArray bChildren(engine());
				b->allChildren(bChildren, Syntax::baseProd(b->production()));

				Nat to = min(aChildren.count(), bChildren.count());
				Priority result = equal;
				for (Nat i = 0; i < to && result == equal; i++) {
					result = aChildren[i]->priority(bChildren[i], syntax);
				}

				if (result != equal)
					return result;

				// The longest one wins. This makes * and + greedy.
				if (aChildren.count() != bChildren.count())
					return aChildren.count() > bChildren.count() ? higher : lower;

				// Nothing more to compare, they are equal to us.
				return equal;
			}

			void TreeNode::allChildren(TreeArray &out, Nat productionId) {
				if (!children)
					return;

				// TODO? Make this iterative in some cases, can be done like in Parser::subtree.
				for (Nat i = 0; i < children->count; i++) {
					TreeNode *child = children->v[i];

					if (!child->addMe(out, productionId))
						out.push(child);
				}
			}

			bool TreeNode::addMe(TreeArray &out, Nat productionId) {
				if (!children)
					return false;
				if (Syntax::baseProd(production()) != productionId)
					return false;
				if (Syntax::specialProd(production()) == 0)
					return false;

				allChildren(out, productionId);
				return true;
			}

			Bool TreeNode::contains(TreeNode *other) const {
				if (other == this)
					return true;

				if (children) {
					for (Nat i = 0; i < children->count; i++) {
						TreeNode *child = children->v[i];
						if (child->pos > pos)
							return false;
						if (child->contains(other))
							return true;
					}
				}

				return false;
			}

		}
	}
}
