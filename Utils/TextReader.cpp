#include "stdafx.h"

#include "TextFile.h"

#include <sstream>

TextReader::TextReader(Stream *stream) : stream(stream) {}

TextReader::~TextReader() {
	if (stream) delete stream;
}

TextReader *TextReader::create(Stream *stream) {
	wchar bom;
	stream->seek(0);
	stream->read(sizeof(wchar), &bom);

	if (bom == 0xFEFF) {
		return new textfile::Utf16Reader(stream, false);
	} else if (bom == 0xFFFE) {
		return new textfile::Utf16Reader(stream, true);
	} else {
		bool bom = false;
		stream->seek(0);
		byte b[3];
		stream->read(3, b);
		if (b[0] == 0xEF && b[1] == 0xBB && b[2] == 0xBF) {
			bom = true;
		} else {
			stream->seek(0);
		}
		return new textfile::Utf8Reader(stream, bom);
	}
}

String TextReader::getLine() {
	std::wostringstream s;

	while (!atEnd()) {
		wchar_t ch = get();
		if (ch == '\n') break;
		else if (ch != '\r') s << ch;
	}

	return String(s.str());
}

String TextReader::getAll() {
	std::wostringstream s;

	while (!atEnd())
		s << get();

	return s.str();
}

//////////////////////////////////////////////////////////////////////////
// Utf8
//////////////////////////////////////////////////////////////////////////

textfile::Utf8Reader::Utf8Reader(Stream *stream, bool bom) : TextReader(stream), bom(bom), nextValid(false) {}

textfile::Utf8Reader::~Utf8Reader() {}

wchar_t textfile::Utf8Reader::get() {
	if (nextValid) {
		nextValid = false;
		return nextCh;
	} else {
		return readChar();
	}
}

wchar_t textfile::Utf8Reader::readChar() {
	nat decoded = decodeUtf8();
	return encodeUtf16(decoded);
}

wchar_t textfile::Utf8Reader::encodeUtf16(nat decoded) {
#ifdef WINDOWS
	//If the value can be stored as a single code unit, do it!
	if (decoded < 0xFFFF) return wchar_t(decoded);

	//Subtract 0x10000 from the code point.
	decoded -= 0x10000;

	//The first code unit is formed from the first ten bits of the modified value.
	wchar_t firstByte = 0xD800 + ((0xFFC00 & decoded) >> 10);

	//The second code unit is formed from the last ten bits of the modified value.
	nextCh = 0xDC00 + (0x3FF & decoded);
	nextValid = true;

	return firstByte;
#else
	// On POSIX, we assume that a wchar_t is UTF-32, which makes life easy:
	return decoded;
#endif
}

void textfile::Utf8Reader::seek(nat position) {
	TextReader::seek(position);
	nextValid = false;
}

nat textfile::Utf8Reader::decodeUtf8() {
	byte firstCh = read();

	if ((firstCh & 0x80) == 0) {
		return firstCh;
	} else if ((firstCh & 0xE0) == 0xC0) {
		return readChar(firstCh & 0x1F, 1);
	} else if ((firstCh & 0xF0) == 0xE0) {
		return readChar(firstCh & 0x0F, 2);
	} else if ((firstCh & 0xF8) == 0xF0) {
		return readChar(firstCh & 0x07, 3);
	} else if ((firstCh & 0xFC) == 0xF8) {
		return readChar(firstCh & 0x03, 4);
	} else if ((firstCh & 0xFE) == 0xFC) {
		return readChar(firstCh & 0x01, 5);
	} else {
		return firstCh; //Invalid
	}
}

nat textfile::Utf8Reader::readChar(byte first, nat numToRead) {
	nat data = first;
	for (nat i = 0; i < numToRead; i++) {
		if (atEnd()) break;

		byte r = peek();
		if ((r & 0xC0) != 0x80) break;
		read();
		data = (data << 6) | (r & 0x3F);
	}
	return data;
}

byte textfile::Utf8Reader::read() {
	byte b = 0;
	stream->read(1, &b);
	return b;
}

byte textfile::Utf8Reader::peek() {
	nat64 p = stream->pos();
	byte b = read();
	stream->seek(p);
	return b;
}

//////////////////////////////////////////////////////////////////////////
// Utf16
//////////////////////////////////////////////////////////////////////////

textfile::Utf16Reader::Utf16Reader(Stream *stream, bool reverseEndian) : TextReader(stream), reverseEndian(reverseEndian) {}

textfile::Utf16Reader::~Utf16Reader() {}

wchar_t textfile::Utf16Reader::get() {
	wchar ch = 0;
	stream->read(sizeof(wchar), &ch);

	if (reverseEndian) {
		ch = ((ch & 0x00FF) << 8) | ((ch & 0xFF00) >> 8);
	}

#ifdef POSIX
	// wchar_t is a UTF-32 character, so we might need to read another character!
	if (ch >= 0xD800 && ch <= 0xDBFF) {
		// Surrogate pair!
		wchar next = 0;
		stream->read(sizeof(wchar), &ch);

		if (reverseEndian) {
			next = ((next & 0x00FF) << 8) | ((next & 0xFF00) >> 8);
		}

		if (next < 0xDC00 || next > 0xDFFF)
			return L'?';

		wchar_t full = (ch & 0x03FF) << 10;
		full |= (next & 0x03FF);
		full += 0x010000;
		return full;
	}
#endif

	return ch;
}


