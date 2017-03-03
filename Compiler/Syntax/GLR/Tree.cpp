#include "stdafx.h"
#include "Tree.h"
#include "Core/StrBuf.h"

namespace storm {
	namespace syntax {
		namespace glr {

			const Nat countMask = 0x80000000;

			void TreeNode::replace(const TreeNode &other) {
				pos(other.pos());
				// Re-point our array.
				write(src, ptr + 1, other.children().ptr);
			}

			/**
			 * Storage of trees.
			 */

			TreeStore::TreeStore(Syntax *syntax) : count(1), syntax(syntax) {
				chunks = runtime::allocArray<Chunk *>(engine(), &pointerArrayType, 16);
			}

			TreeNode TreeStore::push(Nat pos) {
				Nat start = alloc(2);
				write(start, pos);
				write(start + 1, 0);
				return TreeNode(this, start);
			}

			TreeNode TreeStore::push(Nat pos, Nat production, Nat children) {
				Nat start = alloc(3 + children);
				write(start, pos);
				write(start + 1, countMask | children);
				write(start + 2, production);
				return TreeNode(this, start);
			}

			Nat TreeStore::alloc(Nat n) {
				Nat first = chunkId(count);
				Nat last = chunkId(count + n - 1);
				while (last >= chunks->count)
					grow();

				for (Nat i = first; i <= last; i++)
					if (!chunks->v[i])
						chunks->v[i] = runtime::allocArray<Nat>(engine(), &natArrayType, chunkSize);

				Nat r = count;
				count += n;
				return r;
			}

			void TreeStore::grow() {
				GcArray<Chunk *> *n = runtime::allocArray<Chunk *>(engine(), &pointerArrayType, chunks->count * 2);
				memcpy(n->v, chunks->v, sizeof(Chunk *)*chunks->count);
				chunks = n;
			}

			TreeStore::Priority TreeStore::priority(Nat aId, Nat bId) {
				if (aId == bId)
					return equal;

				TreeNode &a = at(aId);
				TreeNode &b = at(bId);
				if (!a.children() || !b.children())
					return equal;

				// These productions are introduced in order to fix epsilon regexes. Do not compare!
				if (Syntax::specialProd(a.production()) == Syntax::prodESkip ||
					Syntax::specialProd(b.production()) == Syntax::prodESkip)
					return equal;

				if (a.pos() != b.pos())
					return a.pos() < b.pos() ? higher : lower;

				Production *aProd = syntax->production(a.production());
				Production *bProd = syntax->production(b.production());
				if (aProd->priority != bProd->priority)
					return aProd->priority > bProd->priority ? higher : lower;
				if (aProd != bProd)
					// If two different productions have the same priority, the behaviour is undefined.
					return equal;

				// Traverse and do a lexiographic compare between the two trees.
				ChildArray aChildren(engine());
				allChildren(aChildren, Syntax::baseProd(a.production()), a);
				ChildArray bChildren(engine());
				allChildren(bChildren, Syntax::baseProd(b.production()), a);

				Nat to = min(aChildren.count(), bChildren.count());
				Priority result = equal;
				for (Nat i = 0; i < to; i++) {
					result = priority(aChildren[i], bChildren[i]);
					if (result != equal)
						return result;
				}

				// The longest one wins. This makes * and + greedy.
				if (aChildren.count() != bChildren.count())
					return aChildren.count() > bChildren.count() ? higher : lower;

				// Nothing more to compare, they look equal to us!
				return equal;
			}

			void TreeStore::allChildren(ChildArray &out, Nat productionId, TreeNode &me) {
				if (!me.children())
					return;

				// TODO? Make this iterative in some cases, can be done like in Parser::subtree.
				TreeArray children = me.children();
				for (Nat i = 0; i < children.count(); i++) {
					Nat child = children[i];

					if (!addNode(out, productionId, child))
						out.push(child);
				}
			}

			bool TreeStore::addNode(ChildArray &out, Nat productionId, Nat node) {
				TreeNode &me = at(node);
				if (!me.children())
					return false;
				if (Syntax::baseProd(me.production()) != productionId)
					return false;
				if (Syntax::specialProd(me.production()) == 0)
					return false;

				allChildren(out, productionId, me);
				return true;
			}

		}
	}
}
