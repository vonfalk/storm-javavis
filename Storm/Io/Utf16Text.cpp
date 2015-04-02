#include "stdafx.h"
#include "Utf16Text.h"
#include "Utils/Endian.h"

namespace storm {

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

}
