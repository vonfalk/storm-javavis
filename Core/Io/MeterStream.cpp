#include "stdafx.h"
#include "MeterStream.h"

namespace storm {

	MeterOStream::MeterOStream(OStream *to) : to(to), pos(0) {}

	void MeterOStream::write(Buffer buf, Nat start) {
		Nat c = 0;
		if (start <= buf.filled())
			c = buf.filled() - start;
		to->write(buf, start);
		pos += c;
	}

	void MeterOStream::flush() {
		to->flush();
	}

	void MeterOStream::close() {
		to->close();
	}

	Word MeterOStream::tell() {
		return pos;
	}

	void MeterOStream::reset() {
		pos = 0;
	}

}
