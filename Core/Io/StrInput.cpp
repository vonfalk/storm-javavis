#include "stdafx.h"
#include "StrInput.h"

namespace storm {

	StrInput::StrInput(Str *src) : pos(src->begin()), end(src->end()) {}

	Char StrInput::readChar() {
		if (pos == end) {
			return Char(nat(0));
		} else {
			Char c = pos.v();
			++pos;
			return c;
		}
	}

}
