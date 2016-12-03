#include "stdafx.h"
#include "Utf8Text.h"

namespace storm {

	Utf8Reader::Utf8Reader(IStream *src) : src(src), buf(), pos(0) {}

	Utf8Reader::Utf8Reader(IStream *src, Buffer start) : src(src), buf(), pos(0) {
		buf = buffer(engine(), bufSize);
		buf.filled(start.filled());
		memcpy(buf.dataPtr(), start.dataPtr(), start.filled());
	}

	// See if the byte is a continuation byte.
	static inline bool isCont(byte c) {
		return (c & 0xC0) == 0x80;
	}

	// The data of the first UTF8-byte.
	static inline void firstData(byte c, nat &remaining, nat &data) {
		if ((c & 0x80) == 0) {
			remaining = 0;
			data = nat(c);
		} else if ((c & 0xC0) == 0x80) {
			// Continuation from before: error!
			remaining = 0;
			data = nat('?');
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
		nat left, r;
		firstData(ch, left, r);

		for (nat i = 0; i < left; i++) {
			ch = readByte();

			if (isCont(ch)) {
				r = (r << 6) | (ch & 0x3F);
			} else {
				// Invalid codepoint, unget ch and return '?'
				ungetByte();
				return Char(nat('?'));
			}
		}

		return Char(r);
	}

	Byte Utf8Reader::readByte() {
		if (buf.count() == 0) {
			buf = src->read(bufSize);
			pos = 0;
		}
		if (pos >= buf.filled()) {
			buf = src->read(buf);
			pos = 0;
		}
		if (pos < buf.filled())
			return buf[pos++];
		else
			return 0;
	}

	void Utf8Reader::ungetByte() {
		if (pos > 0)
			pos--;
	}

}
