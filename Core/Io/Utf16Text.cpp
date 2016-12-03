#include "stdafx.h"
#include "Utf16Text.h"
#include "Core/Utf.h"

namespace storm {

	Utf16Reader::Utf16Reader(IStream *src, Bool byteSwap) : src(src), byteSwap(byteSwap), pos(0) {}

	Utf16Reader::Utf16Reader(IStream *src, Bool byteSwap, Buffer start) : src(src), byteSwap(byteSwap), pos(0) {
		buf = buffer(engine(), bufSize);
		buf.filled(start.filled());
		memcpy(buf.dataPtr(), start.dataPtr(), start.filled());
	}

	Char Utf16Reader::readChar() {
		using namespace utf16;

		nat16 ch = readCh();

		if (leading(ch)) {
			nat16 t = readCh();
			if (trailing(t))
				return Char(assemble(ch, t));
			else
				return Char('?');
		} else if (!trailing(ch)) {
			return Char(Nat(ch));
		} else {
			return Char('?');
		}
	}

	nat16 Utf16Reader::readCh() {
		// Read in network order.
		if (byteSwap) {
			nat16 r = readByte();
			r |= nat16(readByte()) << 8;
			return r;
		} else {
			nat16 r = nat16(readByte()) << 8;
			r |= readByte();
			return r;
		}
	}

	byte Utf16Reader::readByte() {
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

}
