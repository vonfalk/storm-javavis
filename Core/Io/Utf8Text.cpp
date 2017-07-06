#include "stdafx.h"
#include "Utf8Text.h"
#include "Utf.h"

namespace storm {

	Utf8Input::Utf8Input(IStream *src) : src(src), buf(), pos(0) {}

	Utf8Input::Utf8Input(IStream *src, Buffer start) : src(src), buf(), pos(0) {
		buf = buffer(engine(), max(Nat(bufSize), start.filled()));
		buf.filled(start.filled());
		memcpy(buf.dataPtr(), start.dataPtr(), start.filled());
	}

	Char Utf8Input::readChar() {
		byte ch = readByte();
		nat left;
		nat r = utf8::firstData(ch, left);

		for (nat i = 0; i < left; i++) {
			ch = readByte();

			if (utf8::isCont(ch)) {
				r = utf8::addCont(r, ch);
			} else {
				// Invalid codepoint, unget ch and return '?'
				ungetByte();
				return Char(replacementChar);
			}
		}

		return Char(r);
	}

	Byte Utf8Input::readByte() {
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

	void Utf8Input::ungetByte() {
		if (pos > 0)
			pos--;
	}

	/**
	 * Write.
	 */

	Utf8Output::Utf8Output(OStream *to) : TextOutput(), dest(to) {
		init();
	}

	Utf8Output::Utf8Output(OStream *to, TextInfo info) : TextOutput(info), dest(to) {
		init();
	}

	void Utf8Output::init() {
		buf = buffer(engine(), bufSize);
		buf.filled(0);
	}

	void Utf8Output::flush() {
		if (buf.filled() > 0)
			dest->write(buf);
		buf.filled(0);
	}

	void Utf8Output::writeChar(Char ch) {
		Nat cp = ch.codepoint();
		byte out[utf8::maxBytes];
		nat bytes = 0;

		byte *at = utf8::encode(cp, out, &bytes);
		writeBytes(at, bytes);
	}

	void Utf8Output::writeBytes(const byte *data, Nat count) {
		Nat filled = buf.filled();
		if (filled + count >= buf.count()) {
			flush();
			filled = buf.filled();
		}

		memcpy(buf.dataPtr() + filled, data, count);
		buf.filled(filled + count);
	}

}
