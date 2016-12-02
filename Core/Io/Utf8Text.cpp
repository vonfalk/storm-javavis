#include "stdafx.h"
#include "Utf8Text.h"

namespace storm {

	Utf8Reader::Utf8Reader(IStream *src) : src(src), buf(), pos(0) {}

	// First byte of UTF8?
	static inline bool isFirst(byte c) {
		return (c & 0x80) == 0x00  // single byte
			|| (c & 0xC0) == 0xC0; // starting byte
	}

	// The data of the first UTF8-byte.
	static inline void firstData(byte c, nat &remaining, nat &data) {
		if ((c & 0x80) == 0) {
			remaining = 0;
			data = nat(c);
		} else if ((c & 0xE0) == 0xC0) {
			remaining = 1;
			data = nat(c & 0x1F);
		} else if ((c & 0xF0) == 0xE0) {
			remaining = 2;
			data = nat(c & 0x0F);
		} else if ((c & 0xF8) == 0xF0) {
			remaining = 3;
			data = nat(c & 0x07);
		} else if ((c & 0xFC) == 0xF8) {
			remaining = 4;
			data = nat(c & 0x03);
		} else if ((c & 0xFE) == 0xFC) {
			remaining = 5;
			data = nat(c & 0x01);
		} else {
			remaining = 0;
			data = nat('?');
		}
	}

	Char Utf8Reader::readChar() {
		byte ch = readByte();
		if (ch == 0)
			return Char(nat(0));

		if (!isFirst(ch))
			return Char('?');

		nat left, r;
		firstData(ch, left, r);

		for (nat i = 0; i < left; i++) {
			ch = readByte();
			if (isFirst(ch))
				// Sadly, we miss to output a ? here.
				return Char(nat(ch));
			r = (r << 6) | (ch & 0x3F);
		}

		return Char(r);
	}

	Byte Utf8Reader::readByte() {
		if (buf.count() == 0) {
			buf = src->read(1024);
			pos = 0;
		}
		if (buf.filled() >= pos) {
			buf = src->read(buf);
			pos = 0;
		}
		if (buf.filled() < pos)
			return buf[pos++];
		else
			return 0;
	}

}
