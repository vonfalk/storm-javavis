#include "stdafx.h"
#include "File.h"
#include "Engine.h"

namespace storm {
	namespace server {
		using namespace storm::syntax;

		Range::Range() : from(0), to(0) {}

		Range::Range(Nat from, Nat to) {
			this->from = min(from, to);
			this->to = max(from, to);
		}

		Bool Range::intersects(Range other) const {
			return to > other.from
				&& from < other.to;
		}

		StrBuf &operator <<(StrBuf &to, Range r) {
			return to << L"(" << r.from << L" - " << r.to << L")";
		}

		Range combine(Range a, Range b) {
			if (a.empty())
				return b;
			if (b.empty())
				return a;
			return Range(min(a.from, b.from),
						max(a.to, b.to));
		}

		ColoredRange::ColoredRange(Range r, TokenColor c)
			: range(r), color(c) {}

		StrBuf &operator <<(StrBuf &to, ColoredRange r) {
			return to << r.range << L"#" << name(to.engine(), r.color);
		}

		static Nat strlen(Str *s) {
			Nat len = 0;
			for (Str::Iter i = s->begin(), e = s->end(); i != e; ++i)
				len++;
			return len;
		}

		File::File(Nat id, Url *path, Str *src)
			: id(id), path(path), content(null) {

			Engine &e = engine();
			Package *pkg = e.package(path->parent());
			if (!pkg) {
				WARNING(L"The file " << path << L" is not in a package known by Storm. Using a dummy package.");
				pkg = new (this) Package(new (this) Str(L"dummy package"));
			}

			PkgReader *pkgReader = createReader(new (this) Array<Url *>(1, path), pkg);
			FileReader *reader = null;
			if (pkgReader)
				reader = pkgReader->readFile(path);

			if (reader) {
				content = new (this) InfoLeaf(null, src);
				parser = reader->createParser();
				if (InfoNode *n = parse(content, parser->root()))
					content = n;
			}
		}

		Range File::full() {
			if (!content)
				return Range(0, 0);

			return Range(0, content->length());
		}

		Range File::replace(Range range, Str *replace) {
			if (!content)
				return Range(0, 0);

			// Do all regexes match?
			Bool regexMatching = true;

			// Anything to remove?
			if (!range.empty())
				regexMatching &= remove(range);

			// Anything to add?
			if (replace->begin() != replace->end())
				regexMatching &= insert(range.from, replace);

			Range result(range.from, range.from + strlen(replace));
			if (!regexMatching) {
				result = combine(result, parse(range));
			}

			return result;
		}

		Bool File::remove(const Range &range) {
			return remove(range, 0, content);
		}

		Bool File::remove(const Range &range, Nat offset, InfoNode *node) {
			Nat len = node->length();
			if (len == 0)
				return true;

			Range nodeRange(offset, offset + len);
			if (!nodeRange.intersects(range))
				return true;

			if (InfoLeaf *leaf = as<InfoLeaf>(node)) {
				return removeLeaf(range, offset, leaf);
			} else if (InfoInternal *inode = as<InfoInternal>(node)) {
				return removeInternal(range, offset, inode);
			} else {
				assert(false, L"This should not happen!");
				return false; // Force a re-parse.
			}
		}

		Bool File::removeLeaf(const Range &range, Nat offset, InfoLeaf *node) {
			Str *src = node->toS();
			Str::Iter begin = src->begin();
			for (Nat i = offset; i < range.from; i++)
				++begin;

			Str::Iter end = begin;
			for (Nat i = max(offset, range.from); i < range.to; i++)
				++end;

			node->set(src->remove(begin, end));

			return node->matchesRegex();
		}

		Bool File::removeInternal(const Range &range, Nat offset, InfoInternal *node) {
			Bool ok = true;

			for (Nat i = 0; i < node->count(); i++) {
				InfoNode *at = node->at(i);
				// Keep track of the old length, as that will likely change.
				Nat len = at->length();
				ok &= remove(range, offset, at);
				offset += len;
			}

			return ok;
		}

		struct File::InsertState {
			// Insert 'v' at 'point'.
			Nat point;
			Str *v;

			// Anything inserted so far?
			Bool done;

			// First viable match. Maybe not chosen due to regex mismatch.
			InfoLeaf *firstPossible;
			Str *firstPossibleStr;
		};

		Bool File::insert(Nat point, Str *v) {
			InsertState state = {
				point,
				v,
				false,
				null,
			};
			insert(state, 0, content);

			// If 'done' is true, then we succeeded at inserting the string somewhere where the
			// regex matched.
			if (state.done)
				return true;

			// We should insert it at 'firstPossible'.
			assert(state.firstPossible, L"No node for insertion was found!");
			if (!state.firstPossible)
				return false; // Force a re-parse.

			// We can not avoid invalidating a regex...
			state.firstPossible->set(state.firstPossibleStr);
			return false;
		}

		void File::insert(InsertState &state, Nat offset, InfoNode *node) {
			// Outside this node?
			Nat len = node->length();
			if (state.point < offset || state.point > offset + len)
				return;

			if (InfoLeaf *leaf = as<InfoLeaf>(node)) {
				insertLeaf(state, offset, leaf);
			} else if (InfoInternal *inode = as<InfoInternal>(node)) {
				insertInternal(state, offset, inode);
			} else {
				assert(false, L"This should not happen!");
			}
		}

		void File::insertLeaf(InsertState &state, Nat offset, InfoLeaf *node) {
			Str *src = node->toS();
			Str::Iter p = src->begin();
			for (Nat i = offset; i < state.point; i++)
				++p;

			Str *modified = src->insert(p, state.v);
			if (!state.firstPossible) {
				state.firstPossible = node;
				state.firstPossibleStr = modified;
			}

			if (node->matchesRegex(modified)) {
				node->set(modified);
				state.done = true;
			}
		}

		void File::insertInternal(InsertState &state, Nat offset, InfoInternal *node) {
			for (Nat i = 0; i < node->count() && !state.done; i++) {
				InfoNode *at = node->at(i);
				Nat len = at->length();
				insert(state, offset, at);
				offset += len;
			}
		}

		Array<ColoredRange> *File::colors(Range range) {
			Array<ColoredRange> *r = new (this) Array<ColoredRange>();
			if (content)
				colors(r, range, 0, content);
			return r;
		}

		void File::colors(Array<ColoredRange> *out, const Range &range, Nat offset, InfoNode *node) {
			// We can ignore zero-length nodes.
			Nat len = node->length();
			if (len == 0)
				return;

			Range nodeRange(offset, offset + len);
			if (!nodeRange.intersects(range))
				return;

			// Pick the root-most color.
			if (node->color != syntax::tNone) {
				out->push(ColoredRange(nodeRange, node->color));
				return;
			}

			// Else, continue traversing.
			if (InfoInternal *n = as<InfoInternal>(node)) {
				Nat nodeOffset = offset;
				for (Nat i = 0; i < n->count(); i++) {
					colors(out, range, nodeOffset, n->at(i));
					nodeOffset += n->at(i)->length();
				}
			}
		}

		InfoNode *File::parse(InfoNode *node, Rule *root) {
			parser->root(root);
			// TODO: Do our best to do error recovery when parsing like this!
			Str *src = node->toS();
			if (!parser->parse(src, path))
				return null;

			if (parser->matchEnd() != src->end())
				return null;

			InfoNode *r = parser->infoTree();
			parser->clear();
			return r;
		}

		Range File::parse(const Range &range) {
			// TODO: We need some heurustic on how much we should try to re-parse in case we need
			// error recovery. We probably want to try a few parent nodes if error recovery has to
			// kick in near the leaves.
			// TODO: Limit the depth of what needs to be re-parsed in some situations.
			Range result;
			parse(result, range, 0, content);
			return result;
		}

		Bool File::parse(Range &result, const Range &range, Nat offset, InfoNode *node) {
			// We should probably parse at a higher level...
			Nat len = node->length();
			if (len == 0)
				return false;

			// Does this node completely cover 'range'?
			Range nodeRange(offset, offset + len);
			if (nodeRange.from > range.from || nodeRange.to < range.to)
				return false;

			// Do not attempt to re-parse leaf nodes. They only contain regexes, which have been
			// checked already.
			InfoInternal *inode = as<InfoInternal>(node);
			if (!inode)
				return false;


			// Re-parse this node if none of our children have already managed to do so.
			Bool ok = false;
			for (Nat i = 0; i < inode->count() && !ok; i++) {
				InfoNode *child = inode->at(i);
				ok |= parse(result, range, offset, child);
				offset += child->length();
			}

			if (!ok && inode->production()) {
				// Re-parse this node!
				InfoNode *n = parse(inode, inode->production()->rule());
				if (n) {
					replace(inode, n);
					result = nodeRange;
					ok = true;
				}
			}
			return ok;
		}

		void File::replace(InfoNode *oldNode, InfoNode *newNode) {
			InfoInternal *parent = oldNode->parent();
			if (!parent) {
				assert(oldNode == content, L"A non-root node without a parent found.");
				content = newNode;
			} else {
				for (Nat i = 0; i < parent->count(); i++) {
					if (parent->at(i) == oldNode) {
						parent->set(i, newNode);
						return;
					}
				}

				assert(false, L"The child node was not present in our parent.");
			}
		}

		void File::debugOutput(TextOutput *to, Bool tree) const {
			if (!content) {
				to->writeLine(TO_S(this, L"No content for " << id));
			} else if (tree) {
				to->writeLine(TO_S(this, L"Tree of " << id << L":\n" << content->format()));
			} else {
				to->writeLine(TO_S(this, L"Contents of " << id << L":\n" << content));
			}
		}

	}
}
