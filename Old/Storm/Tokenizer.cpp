#include "stdafx.h"
#include "Tokenizer.h"
#include "Exception.h"
#include "Shared/Io/Url.h"

namespace storm {

	wostream &operator <<(wostream &to, const Token &token) {
		return to << token.token;
	}

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
	static const wchar *specials = L"(){}[],;:@";

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

	Tokenizer::Tokenizer(Par<Url> path, const String &src, nat start)
		: src(src), srcFile(path), pos(start), nextToken(L"", SrcPos()) {
		nextToken = findNext();
	}

	Token Tokenizer::next() {
		Token t = peek();
		skip();
		return t;
	}

	void Tokenizer::skip() {
		nextToken = findNext();
	}

	Token Tokenizer::peek() {
		if (!more())
			throw SyntaxError(SrcPos(srcFile, pos), L"Unexpected end of file");

		return nextToken;
	}

	void Tokenizer::expect(const String &tok) {
		if (!more())
			throw SyntaxError(SrcPos(srcFile, pos), L"Unexpected end of file");

		if (nextToken.token != tok)
			throw SyntaxError(nextToken.pos, L"Expected " + tok + L" but got " + nextToken.token);

		// Skip ahead...
		skip();
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

		if (ch == '/' && pos+1 < src.size() && src[pos+1] == ch) {
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

