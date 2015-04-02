#include "stdafx.h"
#include "Utf8Text.h"

namespace storm {

	static inline bool isFirst(byte c) {
		return (c & 0x80) == 0x00 // single byte
			|| (c & 0xC0) == 0xC0; // starting byte
	}

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
		} else if ((c & 0xFE) == 0xFE) {
			remaining = 5;
			data = nat(c & 0x01);
		} else {
			data = nat('?');
			remaining = 0;
		}
	}

	Utf8Reader::Utf8Reader(Par<IStream> stream) : src(stream) {}

	byte Utf8Reader::readCh() {
		byte ch = 0;
		if (src->read(Buffer(&ch, 1)) != 1)
			return 0;
		return ch;
	}

	Nat Utf8Reader::readPoint() {
		byte ch = readCh();
		if (ch == 0)
			return 0;

		if (!isFirst(ch))
			return Nat('?');

		nat left, r;
		firstData(ch, left, r);

		for (nat i = 0; i < left; i++) {
			ch = readCh();
			// Sadly, we miss one here...
			if (isFirst(ch))
				return Nat('?');
			r = (r << 6) | (ch & 0x3F);
		}

		return r;
	}

}
