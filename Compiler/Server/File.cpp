#include "stdafx.h"
#include "File.h"

namespace storm {
	namespace server {
		using namespace storm::syntax;

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

		ColoredRange::ColoredRange(Range r, TokenColor c)
			: range(r), color(c) {}

		StrBuf &operator <<(StrBuf &to, ColoredRange r) {
			return to << r.range << L"#" << name(to.engine(), r.color);
		}

		File::File(Nat id, Url *path, Str *src)
			: id(id), path(path), content(null) {

			Engine &e = engine();
			TODO(L"Find the real package for 'path'.");
			Package *pkg = new (this) Package(new (this) Str(L"dummy package"));
			PkgReader *pkgReader = createReader(new (this) Array<Url *>(1, path), pkg);
			FileReader *reader = null;
			if (pkgReader)
				reader = pkgReader->readFile(path);

			if (reader) {
				content = new (this) InfoLeaf(src);
				parser = reader->createParser();
				content = parse(content, parser->root());
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

			// Anything to remove?
			if (!range.empty())
				remove(range, 0, content);

			// Anything to add?
			if (replace->begin() != replace->end())
				insert(range.from, replace, 0, content);

			return Range(0, 0);
		}

		void File::remove(const Range &range, Nat offset, InfoNode *node) {
			Nat len = node->length();
			if (len == 0)
				return;

			Range nodeRange(offset, offset + len);
			if (!nodeRange.intersects(range))
				return;

			if (InfoLeaf *leaf = as<InfoLeaf>(node)) {
				removeLeaf(range, offset, leaf);
			} else if (InfoInternal *inode = as<InfoInternal>(node)) {
				removeInternal(range, offset, inode);
			} else {
				assert(false, L"This should not happen!");
			}
		}

		void File::removeLeaf(const Range &range, Nat offset, InfoLeaf *node) {
			Str *src = node->toS();
			Str::Iter begin = src->begin();
			for (Nat i = offset; i < range.from; i++)
				++begin;

			Str::Iter end = begin;
			for (Nat i = max(offset, range.from); i < range.to; i++)
				++end;

			node->set(src->remove(begin, end));
		}

		void File::removeInternal(const Range &range, Nat offset, InfoInternal *node) {
			for (Nat i = 0; i < node->count(); i++) {
				InfoNode *at = node->at(i);
				// Keep track of the old length, as that will likely change.
				Nat len = at->length();
				remove(range, offset, at);
				offset += len;
			}
		}

		InfoNode *File::insert(Nat point, Str *v, Nat offset, InfoNode *node) {
			Nat len = node->length();
			if (len == 0)
				return null;

			// Outside this node?
			if (point < offset || point > offset + len)
				return null;

			// TODO? Try to insert inside a node where the regex matches.
			if (InfoLeaf *leaf = as<InfoLeaf>(node)) {
				Str *src = leaf->toS();
				Str::Iter p = src->begin();
				for (Nat i = offset; i < point; i++)
					++p;
				leaf->set(src->insert(p, v));

				return node;
			} else if (InfoInternal *inode = as<InfoInternal>(node)) {
				InfoNode *r = null;

				for (Nat i = 0; i < inode->count() && r == null; i++) {
					InfoNode *at = inode->at(i);
					Nat len = at->length();
					r = insert(point, v, offset, at);
					offset += len;
				}

				return r;
			} else {
				assert(false, L"This should not happen!");
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
				return node;

			if (parser->matchEnd() != src->end())
				return node;

			InfoNode *r = parser->infoTree();
			parser->clear();
			return r;
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
