#include "stdafx.h"
#include "Utf16Text.h"
#include "Core/Utf.h"

namespace storm {

	Utf16Reader::Utf16Reader(IStream *src, Bool byteSwap) : src(src), byteSwap(byteSwap) {}

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
		if (buf.count() < sizeof(nat16))
			buf = buffer(engine(), sizeof(nat16));
		buf = src->read(buf);

		if (buf.empty())
			return 0;
		if (!buf.filled())
			TODO(L"Handle this case!");

		nat16 r = 0;
		if (byteSwap)
			std::swap(buf[0], buf[1]);
		memcpy(&r, buf.dataPtr(), sizeof(r));
		return r;
	}

}
