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

		ColoredRange::ColoredRange(Range r, syntax::TokenColor c)
			: range(r), color(c) {}


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
			return Range(0, 0);
		}

		Array<ColoredRange> *File::colors(Range range) {
			Array<ColoredRange> *r = new (this) Array<ColoredRange>();
			if (content)
				colors(r, range, 0, content);
			return r;
		}

		void File::colors(Array<ColoredRange> *out, const Range &range, Nat offset, syntax::InfoNode *node) {
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
			if (syntax::InfoInternal *n = as<syntax::InfoInternal>(node)) {
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

			return parser->infoTree();
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
