#include "stdafx.h"
#include "Text.h"
#include "StrReader.h"
#include "Utf8Text.h"
#include "Utf16Text.h"
#include "Core/StrBuf.h"

namespace storm {

	TextReader::TextReader() : next(nat(0)), first(true) {}

	Char TextReader::read() {
		Char r = peek();
		next = readChar();
		return r;
	}

	Char TextReader::peek() {
		if (first) {
			first = false;
			next = readChar();
			// See if we need to consume the BOM.
			if (next == Char(nat(0xFEFF)))
				next = readChar();
		}
		return next;
	}

	Bool TextReader::more() {
		if (first)
			peek();
		return next != Char(nat(0));
	}

	Str *TextReader::readLine() {
		StrBuf *to = new (this) StrBuf();

		while (true) {
			Char c = read();
			if (c == Char(nat(0))) {
				break;
			}
			if (c == Char('\r')) {
				c = peek();
				if (c == Char('\n'))
					read();
				break;
			}
			if (c == Char('\n')) {
				break;
			}
			*to << c;
		}

		return to->toS();
	}

	Str *TextReader::readAll() {
		StrBuf *to = new (this) StrBuf();

		while (more()) {
			Char c = read();
			if (c == Char('\r')) {
				c = read();
				// Single \r?
				if (c != Char('\n'))
					*to << L"\n";
			}
			*to << c;
		}

		return to->toS();
	}

	Str *TextReader::readAllRaw() {
		StrBuf *to = new (this) StrBuf();

		while (more()) {
			*to << read();
		}

		return to->toS();
	}

	Char TextReader::readChar() {
		return Char(nat(0));
	}


	TextReader *STORM_FN readText(IStream *stream) {
		nat16 bom = 0;

		Buffer buf = stream->readAll(sizeof(bom));
		if (buf.full()) {
			// Read in network order.
			bom = buf[0] << 8;
			bom |= buf[1];
		}

		if (bom == 0xFEFF)
			return new (stream) Utf16Reader(stream, false, buf);
		else if (bom == 0xFFFE)
			return new (stream) Utf16Reader(stream, true, buf);
		else
			return new (stream) Utf8Reader(stream, buf);
	}

	TextReader *STORM_FN readStr(Str *from) {
		return new (from) StrReader(from);
	}

	Str *STORM_FN readAllText(Url *file) {
		return readText(file->read())->readAll();
	}

}
