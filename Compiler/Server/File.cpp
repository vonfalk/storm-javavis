#include "stdafx.h"
#include "File.h"

namespace storm {
	namespace server {

		Range::Range(Nat from, Nat to) {
			this->from = min(from, to);
			this->to = max(from, to);
		}

		ColoredRange::ColoredRange(Range r, TextColor c)
			: range(r), color(c) {}

		Str *colorName(EnginePtr e, TextColor c) {
			const wchar *name = L"<unknown>";
			switch (c) {
			case tNone:
				name = L"nil";
				break;
			case tComment:
				name = L"comment";
				break;
			case tDelimiter:
				name = L"delimiter";
				break;
			case tString:
				name = L"string";
				break;
			case tConstant:
				name = L"constant";
				break;
			case tKeyword:
				name = L"keyword";
				break;
			case tFnName:
				name = L"fn-name";
				break;
			case tVarName:
				name = L"var-name";
				break;
			case tTypeName:
				name = L"type-name";
				break;
			}

			return new (e.v) Str(name);
		}

		File::File(Nat id, Url *path, Str *content)
			: id(id), path(path), content(content) {}

		Range File::full() {
			Range r(0, 0);
			for (Str::Iter f = content->begin(); f != content->end(); ++f)
				r.to++;
			return r;
		}

		Range File::replace(Range range, Str *replace) {
			Str::Iter f = content->begin();
			for (nat i = 0; i < range.from; i++)
				++f;

			Str::Iter t = content->begin();
			for (nat i = 0; i < range.to; i++)
				++t;

			Str *first = content->substr(content->begin(), f);
			Str *last = content->substr(t);

			content = *(*first + replace) + last;

			// Right now, we just color every two characters, so the entire remainder of the file
			// needs to be invalidated.
			return Range(range.from, full().to);
		}

		Array<ColoredRange> *File::colors(Range range) {
			// This is just a dummy implementation for testing the implementation.

			Str::Iter f = content->begin();
			for (nat i = 0; i < range.from; i++)
				++f;

			Array<ColoredRange> *r = new (this) Array<ColoredRange>();

			for (nat i = range.from; f != content->end() && i < range.to; ++f, ++i) {
				if (i % 4 == 1 && i > 0)
					r->push(ColoredRange(Range(i - 1, i + 1), tString));
			}

			return r;
		}

	}
}
