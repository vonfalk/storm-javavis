#include "stdafx.h"
#include "Text.h"
#include "StrReader.h"
#include "Utf8Text.h"
#include "Utf16Text.h"
#include "Core/StrBuf.h"

namespace storm {

	TextInfo::TextInfo() : useCrLf(false), useBom(false) {}

	TextReader::TextReader() : next(nat(0)), hasNext(false), first(true), eof(false) {}

	Char TextReader::doRead() {
		Char c = readChar();
		if (first && c == Char(Nat(0xFEFF)))
			c = readChar();
		if (c == Char(Nat(0)))
			eof = true;
		return c;
	}

	Char TextReader::read() {
		if (hasNext) {
			hasNext = false;
			return next;
		} else {
			return doRead();
		}
	}

	Char TextReader::peek() {
		if (!hasNext) {
			next = doRead();
			hasNext = true;
		}
		return next;
	}

	Bool TextReader::more() {
		if (hasNext)
			return next != Char(Nat(0));
		return !eof;
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

		while (true) {
			Char c = read();
			if (c == Char(Nat(0)))
				break;
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
			Char c = read();
			if (c == Char(Nat(0)))
				break;
			*to << c;
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


	/**
	 * Write text.
	 */

	TextWriter::TextWriter() : autoFlush(true), config() {}

	TextWriter::TextWriter(TextInfo info) : autoFlush(true), config(info) {}

	void TextWriter::write(Char c) {
		writeBom();
		writeChar(c);
	}

	void TextWriter::write(Str *s) {
		writeBom();

		Char newline('\n');
		for (Str::Iter i = s->begin(), end = s->end(); i != end; ++i) {
			if (i.v() == newline)
				writeLine();
			else
				writeChar(i.v());
		}
	}

	void TextWriter::writeLine() {
		writeBom();

		if (config.useCrLf)
			writeChar(Char('\r'));
		writeChar(Char('\n'));
		if (autoFlush)
			flush();
	}

	void TextWriter::writeLine(Str *s) {
		write(s);
		writeLine();
	}

	void TextWriter::writeBom() {
		if (config.useBom) {
			writeChar(Char(Nat(0xFEFF)));
			config.useBom = false;
		}
	}

	void TextWriter::writeChar(Char ch) {}

	void TextWriter::flush() {}

}
