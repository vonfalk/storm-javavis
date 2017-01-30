#include "stdafx.h"
#include "File.h"

namespace storm {
	namespace server {
		using namespace storm::syntax;

		Range::Range(Nat from, Nat to) {
			this->from = min(from, to);
			this->to = max(from, to);
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
				PVAR(content->format());
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
			// DO STUFF!
			return r;
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

	}
}
