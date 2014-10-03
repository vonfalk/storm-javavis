#include "stdafx.h"
#include "SyntaxToken.h"

namespace storm {

	/**
	 * Regex token.
	 */

	RegexToken::RegexToken(const String &regex, const String &to) : regex(regex) {
		bindTo = to;
	}

	void RegexToken::output(std::wostream &to) const {
		to << '"' << regex << '"';
		if (bindTo != L"")
			to << ' ' << bindTo;
	}


	/**
	 * Type token.
	 */

	TypeToken::TypeToken(const String &name, const String &to) : typeName(name) {
		bindTo = to;
	}

	void TypeToken::output(std::wostream &to) const {
		to << typeName;
		if (bindTo != L"")
			to << ' ' << bindTo;
	}

	/**
	 * Whitespace token.
	 */

	DelimToken::DelimToken() : TypeToken(L"DELIMITER") {}

	void DelimToken::output(std::wostream &to) const {
		to << ',';
	}
}
