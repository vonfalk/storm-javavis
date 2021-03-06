#include "stdafx.h"
#include "Utf16Text.h"
#include "Core/Utf.h"

namespace storm {

	Utf16Input::Utf16Input(IStream *src, Bool byteSwap) : src(src), byteSwap(byteSwap), pos(0) {}

	Utf16Input::Utf16Input(IStream *src, Bool byteSwap, Buffer start) : src(src), byteSwap(byteSwap), pos(0) {
		buf = buffer(engine(), max(Nat(bufSize), start.filled()));
		buf.filled(start.filled());
		memcpy(buf.dataPtr(), start.dataPtr(), start.filled());
	}

	void Utf16Input::close() {
		src->close();
	}

	Char Utf16Input::readChar() {
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

	nat16 Utf16Input::readCh() {
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

	byte Utf16Input::readByte() {
		if (buf.count() == 0) {
			buf = src->read(bufSize);
			pos = 0;
		}
		if (pos >= buf.filled()) {
			if (buf.count() < bufSize) {
				buf = src->read(bufSize);
			} else {
				buf.filled(0);
				buf = src->read(buf);
			}
			pos = 0;
		}
		if (pos < buf.filled())
			return buf[pos++];
		else
			return 0;
	}

	/**
	 * Output.
	 */

	Utf16Output::Utf16Output(OStream *to, Bool byteSwap) : TextOutput(), dest(to), byteSwap(byteSwap) {
		init();
	}

	Utf16Output::Utf16Output(OStream *to, TextInfo info, Bool byteSwap) : TextOutput(info), dest(to), byteSwap(byteSwap) {
		init();
	}

	void Utf16Output::init() {
		buf = buffer(engine(), bufSize);
		buf.filled(0);
	}

	void Utf16Output::flush() {
		if (buf.filled() > 0)
			dest->write(buf);
		buf.filled(0);
	}

	void Utf16Output::close() {
		dest->close();
	}

	void Utf16Output::write(byte *to, wchar ch) {
		if (byteSwap) {
			to[0] = byte(ch & 0xFF);
			to[1] = byte((ch >> 8) & 0xFF);
		} else {
			to[0] = byte((ch >> 8) & 0xFF);
			to[1] = byte(ch & 0xFF);
		}
	}

	void Utf16Output::writeChar(Char ch) {
		byte buf[4];

		if (ch.leading()) {
			write(buf, ch.leading());
			write(buf + 2, ch.trailing());
			writeBytes(buf, 4);
		} else {
			write(buf, ch.trailing());
			writeBytes(buf, 2);
		}
	}

	void Utf16Output::writeBytes(const byte *data, Nat count) {
		Nat filled = buf.filled();
		if (filled + count >= buf.count()) {
			flush();
			filled = buf.filled();
		}

		memcpy(buf.dataPtr() + filled, data, count);
		buf.filled(filled + count);
	}


}
