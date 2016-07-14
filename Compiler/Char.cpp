#include "stdafx.h"
#include "Char.h"
#include "Utf.h"
#include "Str.h"

namespace storm {

	Char::Char(char ch) : value(ch) {}

	Char::Char(wchar ch) : value(ch) {}

	Char::Char(Nat codepoint) : value(codepoint) {}

	Bool Char::operator ==(Char o) const {
		return value == o.value;
	}

	Bool Char::operator !=(Char o) const {
		return value != o.value;
	}

	Nat Char::hash() const {
		return value;
	}

	wchar Char::leading() const {
		return utf16::splitLeading(value);
	}

	wchar Char::trailing() const {
		return utf16::splitTrailing(value);
	}

	nat Char::size() const {
		return leading() == 0 ? 1 : 2;
	}

	void Char::deepCopy(CloneEnv *env) {}

	Str *toS(EnginePtr e, Char ch) {
		return new (e.v) Str(ch);
	}

	wostream &operator <<(wostream &to, const Char &ch) {
		wchar leading = ch.leading();
		if (leading)
			to << leading;
		to << ch.trailing();
		return to;
	}

}
