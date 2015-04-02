#include "stdafx.h"
#include "Text.h"
#include "Utf8Text.h"
#include "Utf16Text.h"
#include "Utils/Endian.h"

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

	void TextWriter::write(Str *str) {
		TODO(L"Convert each character to UTF32 and write() it.");
	}

	void TextWriter::writeLine(Str *str) {
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

}
