#include "stdafx.h"
#include "Utf16Text.h"
#include "Utils/Endian.h"

namespace storm {

	/**
	 * Read.
	 */

	static inline bool leading(nat16 ch) {
		// return (ch >= 0xD800) && (ch < 0xDC00);
		return (ch & 0xFC00) == 0xD800;
	}

	static inline bool trailing(nat16 ch) {
		// return (ch >= 0xDC00) && (ch < 0xE000);
		return (ch & 0xFC00) == 0xDC00;
	}

	static inline nat assemble(nat16 lead, nat16 trail) {
		nat r = nat(lead & 0x3FF) << nat(10);
		r |= nat(trail & 0x3FF);
		r += 0x10000;
		return r;
	}

	Utf16Reader::Utf16Reader(Par<IStream> stream, Bool be) : src(stream), bigEndian(be) {}

	nat16 Utf16Reader::readCh() {
		nat16 ch = 0;
		if (src->read(Buffer(&ch, 2)) != 2)
			return 0;
		networkSwap(ch);
		if (!bigEndian)
			ch = byteSwap(ch);
		return ch;
	}

	Nat Utf16Reader::readPoint() {
		while (nat16 ch = readCh()) {
			if (leading(ch)) {
				nat16 t = readCh();
				if (trailing(t))
					return assemble(ch, t);
				else
					return Nat('?');
			} else if (!trailing(ch)) {
				return Nat(ch);
			}
		}

		return 0;
	}

	/**
	 * Write.
	 */

	Utf16Writer::Utf16Writer(Par<OStream> to, Bool be) : to(to), bigEndian(be) {}

	void Utf16Writer::write(Nat p) {
		if (p >= 0xD800 && p < 0xE000) {
			// Invalid codepoint.
			p = '?';
		}

		if (p >= 0x110000) {
			// Too large for UTF16.
			p = '?';
		}

		if (p < 0xFFFF) {
			// One is enough!
			nat16 o(p);
			networkSwap(o);
			if (bigEndian)
				o = byteSwap(o);

			to->write(Buffer(&o, 2));
		} else {
			// Surrogate pair.
			p -= 0x010000;
			nat16 o[2] = {
				nat16(((p >> 10) & 0x3FF) + 0xD800),
				nat16((p & 0x3FF) + 0xDC00),
			};
			networkSwap(o[0]);
			networkSwap(o[1]);
			if (bigEndian) {
				o[0] = byteSwap(o[0]);
				o[1] = byteSwap(o[1]);
			}

			to->write(Buffer(o, 4));
		}
	}

}
