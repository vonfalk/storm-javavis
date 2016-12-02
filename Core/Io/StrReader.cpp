#include "stdafx.h"
#include "StrReader.h"

namespace storm {

	StrReader::StrReader(Str *src) : pos(src->begin()), end(src->end()) {}

	Char StrReader::readChar() {
		if (pos == end) {
			return Char(nat(0));
		} else {
			Char c = pos.v();
			++pos;
			return c;
		}
	}

}
