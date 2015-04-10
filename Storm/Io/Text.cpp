#include "stdafx.h"
#include "Text.h"
#include "Url.h"
#include "Utf8Text.h"
#include "Utf16Text.h"
#include "Utils/Endian.h"
#include "MemStream.h"

namespace storm {

	TextReader::TextReader() : first(true), next(0) {}

	static void add(std::wostringstream &to, Nat c) {
		if (c >= 0xD800 && c < 0xE000) {
			// Invalid codepoint.
			to << '?';
			return;
		}

		if (c >= 0x110000) {
			// Too large, not defined by UTF16.
			to << '?';
		}

		if (c < 0xFFFF) {
			// One unit will do!
			to << wchar(c);
		} else {
			// We need a surrogate pair.
			c -= 0x010000;
			to << wchar(((c >> 10) & 0x3FF) + 0xD800);
			to << wchar((c & 0x3FF) + 0xDC00);
		}
	}

	Nat TextReader::readPoint() {
		return 0;
	}

	Nat TextReader::read() {
		if (first)
			peek();
		Nat r = next;
		next = readPoint();
		return r;
	}

	Nat TextReader::peek() {
		if (first) {
			first = false;
			next = readPoint();
		}
		return next;
	}

	Str *TextReader::readLine() {
		std::wostringstream o;

		TODO(L"Handle line-endings better!");

		while (Nat ch = read()) {
			if (ch == '\r')
				continue;
			if (ch == '\n')
				break;
			add(o, ch);
		}

		return CREATE(Str, this, o.str());
	}

	Str *TextReader::readAll() {
		std::wostringstream o;

		while (Nat ch = read()) {
			add(o, ch);
		}

		return CREATE(Str, this, o.str());
	}

	TextWriter::TextWriter() {}

	void TextWriter::write(Nat codepoint) {}

	void TextWriter::write(Par<Str> str) {
		using namespace utf16;

		const String &s = str->v;
		nat16 last = 0;
		for (nat i = 0; i < s.size(); i++) {
			wchar ch = s[i];
			if (leading(ch)) {
				last = ch;
			} else if (trailing(ch)) {
				if (leading(last))
					write(assemble(last, ch));
				else
					write('?');
				last = 0;
			} else {
				write(Nat(ch));
				last = 0;
			}
		}
	}

	void TextWriter::writeLine(Par<Str> str) {
		write(str);
		write('\n');
	}

	TextReader *readText(Par<IStream> from) {
		nat16 bom = 0;
		// We do not have to check the return value here, otherwise 'bom' won't be written.
		from->peek(Buffer(&bom, 2));
		networkSwap(bom);

		Auto<TextReader> created;

		if (bom == 0xFEFF) {
			created = CREATE(Utf16Reader, from, from, true);
		} else if (bom == 0xFFFE) {
			created = CREATE(Utf16Reader, from, from, false);
		} else {
			created = CREATE(Utf8Reader, from, from);
		}

		// Consume the BOM if relevant.
		Nat first = created->peek();
		if (first == 0xFEFF)
			created->read();

		return created.ret();
	}

	TextReader *readStr(Par<Str> from) {
		nat16 bom = 0xFEFF;
		nat strSize = from->v.size() * sizeof(from->v[0]);
		Buffer b(strSize + sizeof(bom));
		copyArray(b.dataPtr(), (byte *)&bom, sizeof(bom));
		copyArray(b.dataPtr() + sizeof(bom), (byte *)&from->v[0], strSize);

		Auto<IMemStream> src = CREATE(IMemStream, from, b);
		return readText(src);
	}

	Str *readAllText(Par<Url> from) {
		Auto<IStream> stream = from->read();
		Auto<TextReader> reader = readText(stream);
		return reader->readAll();
	}

}
