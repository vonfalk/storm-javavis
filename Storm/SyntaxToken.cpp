#include "stdafx.h"
#include "SyntaxToken.h"

namespace storm {

	SyntaxToken::SyntaxToken(const String &to, bool method) : bindTo(to), method(method) {}

	void SyntaxToken::output(wostream &to) const {
		output(to, true);
	}

	void SyntaxToken::output(wostream &to, bool bindings) const {
		if (bindings && bindTo != L"") {
			to << ' ';
			if (method)
				to << L"-> ";
			to << bindTo;
		}
	}

	/**
	 * Regex token.
	 */

	RegexToken::RegexToken(const String &regex, const String &to, bool method)
		: SyntaxToken(to, method), regex(regex) {}

	void RegexToken::output(std::wostream &to, bool bindings) const {
		to << '"' << regex << '"';
		SyntaxToken::output(to, bindings);
	}


	/**
	 * Type token.
	 */

	TypeToken::TypeToken(const String &name, const String &to, bool method)
		: SyntaxToken(to, method), typeName(name) {}

	void TypeToken::output(std::wostream &to, bool bindings) const {
		to << typeName;
		if (bindings && params.size() > 0) {
			to << L"(";
			join(to, params, L", ");
			to << L")";
		}
		SyntaxToken::output(to, bindings);
	}

	/**
	 * Whitespace token.
	 */

	DelimToken::DelimToken(const String &name) : TypeToken(name) {}
}
