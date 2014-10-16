#include "stdafx.h"
#include "Tokenizer.h"
#include "Exception.h"

namespace storm {

	bool Token::isStr() const {
		return token.size() >= 2
			&& token[0] == '"'
			&& token[token.size()-1] == '"';
	}

	String Token::strVal() const {
		assert(isStr());
		return token.substr(1, token.size() - 2);
	}

	static const wchar *operators = L"-?*=><.+";
	static const wchar *specials = L"(){},;";

	static bool isOperator(wchar c) {
		for (const wchar *p = operators; *p; p++)
			if (c == *p)
				return true;
		return false;
	}

	static bool isSpecial(wchar c) {
		for (const wchar *p = specials; *p; p++)
			if (c == *p)
				return true;
		return false;
	}

	static bool isWhitespace(wchar c) {
		switch (c) {
		case ' ':
		case '\n':
		case '\r':
		case '\t':
			return true;
		}
		return false;
	}

	Tokenizer::Tokenizer(const Path &path, const String &src, nat start)
		: src(src), srcFile(path), pos(start), nextToken(L"", SrcPos(Path(), 0)) {
		nextToken = findNext();
	}

	Token Tokenizer::next() {
		Token t = peek();
		nextToken = findNext();
		return t;
	}

	Token Tokenizer::peek() {
		if (!more())
			throw SyntaxError(SrcPos(srcFile, pos), L"Unexpected end of file");

		return nextToken;
	}

	bool Tokenizer::more() const {
		return !nextToken.empty();
	}

	Token Tokenizer::findNext() {
		State state = sStart;
		nat start = pos;

		while (state != sDone) {
			processChar(start, state);
		}

		return Token(src.substr(start, pos - start), SrcPos(srcFile, start));
	}

	void Tokenizer::processChar(nat &start, State &state) {
		if (pos >= src.size()) {
			state = sDone;
			return;
		}

		wchar ch = src[pos];

		if (ch == '/' && pos < src.size() && src[pos] == ch) {
			switch (state) {
			case sStart:
				state = sComment;
				break;
			case sString:
			case sComment:
				break;
			default:
				state = sDone;
				return;
			}
		}

		switch (state) {
		case sStart:
			pos++;
			if (isWhitespace(ch)) {
				start = pos;
			} else if (isSpecial(ch)) {
				state = sDone;
			} else if (isOperator(ch)) {
				state = sOperator;
			} else if (ch == '"') {
				state = sString;
			} else {
				state = sText;
			}
			break;
		case sText:
			if (isOperator(ch) || isWhitespace(ch) || isSpecial(ch)) {
				state = sDone;
			} else {
				pos++;
			}
			break;
		case sOperator:
			if (!isOperator(ch)) {
				state = sDone;
			} else {
				pos++;
			}
			break;
		case sString:
			pos++;
			if (ch == '"') {
				state = sDone;
			} else if (ch == '\\') {
				pos++;
			}
			break;
		case sComment:
			start = ++pos;
			if (ch == '\n')
				state = sStart;
			break;
		case sDone:
			pos--;
			break;
		}

	}
}

