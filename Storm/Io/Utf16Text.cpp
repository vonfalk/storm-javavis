#include "stdafx.h"
#include "Utf16Text.h"

namespace storm {

	static inline bool leading(nat16 ch) {
		return (ch & 0xF800) == 0xDB00;
	}

	static inline bool trailing(nat16 ch) {
		return (ch & 0xF800) == 0xDC00;
	}

	static inline nat assemble(nat16 lead, nat16 trail) {
		return nat(lead & 0x7FF) << nat(10)
			| nat(trail & 0x7FF);
	}

	static inline void reverse(nat16 &ch) {
		nat16 r = (ch & 0xFF00) >> 8;
		r |= (ch & 0xFF) << 8;
		ch = r;
	}

	Utf16Reader::Utf16Reader(Par<IStream> stream, Bool rev) : src(stream), rev(rev) {}

	nat16 Utf16Reader::readCh() {
		nat16 ch = 0;
		if (src->read(Buffer(&ch, 2)) != 2)
			return 0;
		if (rev)
			reverse(ch);
		return ch;
	}

	Nat Utf16Reader::read() {
		while (nat16 ch = readCh()) {
			if (leading(ch)) {
				nat16 t = readCh();
				if (trailing(ch))
					return assemble(t, ch);
				else
					return Nat('?');
			} else if (!trailing(ch)) {
				return Nat(ch);
			}
		}

		return 0;
	}

	Bool Utf16Reader::more() {
		return src->more();
	}

}
