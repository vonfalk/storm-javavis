#include "stdafx.h"
#include "SyntaxToken.h"

namespace storm {

	/**
	 * Regex token.
	 */

	RegexToken::RegexToken(const String &regex) : regex(regex) {}

	void RegexToken::output(std::wostream &to) const {
		to << '"' << regex << '"';
	}


	/**
	 * Type token.
	 */

	TypeToken::TypeToken(const String &name, const String &to) : typeName(name), bindTo(to) {}

	void TypeToken::output(std::wostream &to) const {
		to << typeName;
		if (bindTo != L"")
			to << ' ' << bindTo;
	}
}
