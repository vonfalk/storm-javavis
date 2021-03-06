#include "stdafx.h"
#include "Tree.h"
#include "Core/StrBuf.h"

namespace storm {
	namespace syntax {
		namespace glr {

			const Nat countMask = 0x80000000;
			const Nat errorMask = 0x40000000;
			const Nat posMask   = 0x3FFFFFFF;

			void TreeNode::replace(const TreeNode &other) {
				// We only operate on nodes containing children.
				// This can be an issue when nullable terminals are matched as a production.
				Nat attrs = read(src, ptr);
				if ((attrs & countMask) == 0)
					return;
				if ((read(src, other.ptr) & countMask) == 0)
					return;

				pos(other.pos());

				if (attrs & errorMask) {
					// We can store errors!
					write(src, ptr + 1, InfoErrors::getData(other.errors()));
					write(src, ptr + 2, other.children().ptr);
				} else {
					// Re-point our array.
					write(src, ptr + 1, other.children().ptr);
				}
			}

			/**
			 * Storage of trees.
			 */

			TreeStore::TreeStore(Syntax *syntax) : size(1), syntax(syntax) {
				chunks = runtime::allocArray<Chunk *>(engine(), &pointerArrayType, 16);
			}

			TreeNode TreeStore::push(Nat pos) {
				Nat start = alloc(1);
				write(start, pos);
				return TreeNode(this, start);
			}

			TreeNode TreeStore::push(Nat pos, InfoErrors errors) {
				if (InfoErrors::getData(errors) == 0)
					return push(pos);

				Nat start = alloc(2);
				write(start, errorMask | pos);
				write(start + 1, InfoErrors::getData(errors));
				return TreeNode(this, start);
			}

			TreeNode TreeStore::push(Nat pos, Nat production, Nat children) {
				Nat start = alloc(3 + children);
				write(start, countMask | pos);
				write(start + 1, countMask | children);
				write(start + 2, production);
				return TreeNode(this, start);
			}

			TreeNode TreeStore::push(Nat pos, Nat production, InfoErrors errors, Nat children) {
				if (InfoErrors::getData(errors) == 0)
					return push(pos, production, children);

				Nat start = alloc(4 + children);
				write(start, countMask | errorMask | pos);
				write(start + 1, InfoErrors::getData(errors));
				write(start + 2, countMask | children);
				write(start + 3, production);
				return TreeNode(this, start);
			}

			Nat TreeStore::alloc(Nat n) {
				Nat first = chunkId(size);
				Nat last = chunkId(size + n - 1);
				while (last >= chunks->count)
					grow();

				for (Nat i = first; i <= last; i++)
					if (!chunks->v[i])
						chunks->v[i] = runtime::allocArray<Nat>(engine(), &natArrayType, chunkSize);

				lastAlloc = size;
				size += n;
				return lastAlloc;
			}

			void TreeStore::free(Nat alloc) {
				if (lastAlloc == alloc) {
					size = lastAlloc;
				}
			}

			void TreeStore::grow() {
				GcArray<Chunk *> *n = runtime::allocArray<Chunk *>(engine(), &pointerArrayType, chunks->count * 2);
				memcpy(n->v, chunks->v, sizeof(Chunk *)*chunks->count);
				chunks = n;
			}

			TreeStore::Priority TreeStore::priority(Nat aId, Nat bId) {
				if (aId == bId)
					return equal;

				TreeNode a = at(aId);
				TreeNode b = at(bId);

				// Prioritize the one with fewer errors.
				if (a.errors() != b.errors())
					return a.errors() < b.errors() ? higher : lower;

				// No children -> nothing more to compare.
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
				allChildren(bChildren, Syntax::baseProd(b.production()), b);

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

			Bool TreeStore::contains(Nat haystack, Nat needle) {
				if (haystack == needle)
					return true;

				TreeNode in = at(haystack);
				TreeArray children = in.children();
				if (!children)
					return false;

				TreeNode n = at(needle);
				Nat start = in.pos();
				for (Nat i = 0; i < children.count(); i++) {
					TreeNode ch = at(children[i]);
					if (ch.pos() > n.pos())
						return false;

					if (start <= n.pos())
						if (contains(children[i], needle))
							return true;

					start = ch.pos();
				}

				return false;
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
				TreeNode me = at(node);
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
