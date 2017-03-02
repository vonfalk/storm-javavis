#include "stdafx.h"
#include "Tree.h"
#include "Core/StrBuf.h"

namespace storm {
	namespace syntax {
		namespace glr {

			TreeNode::TreeNode(Nat pos) : pos(pos), children(null) {}

			TreeNode::TreeNode(Engine &e, Nat pos, Nat production, Nat count) : pos(pos) {
				children = runtime::allocArray<Nat>(e, &natArrayType, count);
				children->filled = production;
			}


			/**
			 * Storage of trees.
			 */

			TreeStore::TreeStore(Syntax *syntax) : count(1), syntax(syntax) {
				chunks = runtime::allocArray<Chunk *>(engine(), &pointerArrayType, 16);
				arrayType = StormInfo<TreeNode>::handle(engine()).gcArrayType;
			}

			Nat TreeStore::push(const TreeNode &item) {
				Nat id = chunkId(count);
				if (id >= chunks->count)
					grow();

				Chunk *&chunk = chunks->v[id];
				if (!chunk)
					chunk = runtime::allocArray<TreeNode>(engine(), arrayType, chunkSize);

				chunk->v[chunkOffset(count)] = item;
				return count++;
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
				if (!a.children || !b.children)
					return equal;

				// These productions are introduced in order to fix epsilon regexes. Do not compare!
				if (Syntax::specialProd(a.production()) == Syntax::prodESkip ||
					Syntax::specialProd(b.production()) == Syntax::prodESkip)
					return equal;

				if (a.pos != b.pos)
					return a.pos < b.pos ? higher : lower;

				Production *aProd = syntax->production(a.production());
				Production *bProd = syntax->production(b.production());
				if (aProd->priority != bProd->priority)
					return aProd->priority > bProd->priority ? higher : lower;
				if (aProd != bProd)
					// If two different productions have the same priority, the behaviour is undefined.
					return equal;

				// Traverse and do a lexiographic compare between the two trees.
				TreeArray aChildren(engine());
				allChildren(aChildren, Syntax::baseProd(a.production()), a);
				TreeArray bChildren(engine());
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

			void TreeStore::allChildren(TreeArray &out, Nat productionId, TreeNode &me) {
				if (!me.children)
					return;

				// TODO? Make this iterative in some cases, can be done like in Parser::subtree.
				for (Nat i = 0; i < me.children->count; i++) {
					Nat child = me.children->v[i];

					if (!addNode(out, productionId, child))
						out.push(child);
				}
			}

			bool TreeStore::addNode(TreeArray &out, Nat productionId, Nat node) {
				TreeNode &me = at(node);
				if (!me.children)
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
