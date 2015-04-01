#include "stdafx.h"
#include "Text.h"

namespace storm {

	TextReader::TextReader() {}

	Nat TextReader::read() {
		return 0;
	}

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
			to << wchar(((c >> 10) & 0x7FF) + 0xD800);
			to << wchar((c & 0x7FF) + 0xDC00);
		}
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

	Bool TextReader::more() {
		return false;
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

		if (bom == 0xFEFF) {
			return null;
		} else if (bom == 0xFFFE) {
			return null;
		} else {
			return null;
		}
	}

}
