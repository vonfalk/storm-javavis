#include "stdafx.h"
#include "Char.h"
#include "Utf.h"
#include "Str.h"
#include "Hash.h"

namespace storm {

	Char::Char(Int codepoint) : value(codepoint) {}

	Bool Char::operator ==(const Char &o) const {
		return value == o.value;
	}

	Bool Char::operator !=(const Char &o) const {
		return value != o.value;
	}

	Nat Char::hash() const {
		return natHash(value);
	}

	Str *toS(EnginePtr e, Char ch) {
		return CREATE(Str, e.v, ch);
	}

	wostream &operator <<(wostream &to, const Char &ch) {
		if (!utf16::valid(ch.value)) {
			to << '?';
		} else if (utf16::split(ch.value)) {
			to << utf16::splitLeading(ch.value);
			to << utf16::splitTrailing(ch.value);
		} else {
			to << wchar(ch.value);
		}
		return to;
	}

}
