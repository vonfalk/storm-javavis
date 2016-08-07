#include "stdafx.h"
#include "Tokenizer.h"
#include "Exception.h"
#include "Utils/TextReader.h"
#include "Utils/FileStream.h"

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

static const wchar *operators = L"+?*&=.:!";
static const wchar *specials = L"[](){},-;<>~";

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

Tokenizer::Tokenizer(nat fileId)
	: src(readTextFile(SrcPos::files[fileId])), pathId(fileId), pos(0), srcPos(fileId, 0, 0), nextToken(L"", SrcPos()) {
	nextToken = findNext();
}

Token Tokenizer::next() {
	Token t = peek();
	nextToken = findNext();
	return t;
}

void Tokenizer::skip() {
	nextToken = findNext();
}

Token Tokenizer::peek() {
	if (!more())
		throw Error(L"End of file!", srcPos);

	return nextToken;
}

void Tokenizer::expect(const String &t) {
	Token tok = next();
	if (tok.token != t)
		throw Error(L"Expected " + t + L" but got " + tok.token, tok.pos);
}

bool Tokenizer::skipIf(const String &t) {
	Token tok = peek();
	if (tok.token == t) {
		skip();
		return true;
	} else {
		return false;
	}
}

bool Tokenizer::more() const {
	return !nextToken.empty();
}

void Tokenizer::advance(SrcPos &pos, nat from, nat to) const {
	for (nat i = from; i < to; i++) {
		if (src[i] == '\n') {
			pos.line++;
			pos.col = 0;
		} else {
			pos.col++;
		}
	}
}

Token Tokenizer::findNext() {
	State state = sStart;
	nat start = pos;
	nat from = pos; // from may be modified, while start may not.
	SrcPos sStart = srcPos;

	while (state != sDone) {
		processChar(from, state);
	}

	// Advance line-counting.
	advance(sStart, start, from);
	advance(srcPos, start, pos);

	return Token(src.substr(from, pos - from), sStart);
}

void Tokenizer::processChar(nat &start, State &state) {
	if (pos >= src.size()) {
		state = sDone;
		return;
	}

	wchar ch = src[pos];
	nat oldPos = pos;


	if (ch == '/' && pos+1 < src.size() && (src[pos+1] == '/' || src[pos+1] == '*')) {
		switch (state) {
		case sStart:
			if (src[pos+1] == '*')
				state = sMlComment;
			else
				state = sComment;
			break;
		case sString:
		case sComment:
		case sPreproc:
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
		} else if (ch == '#') {
			state = sPreproc;
		} else {
			state = sText;
		}
		break;
	case sText:
		if (isOperator(ch) || isWhitespace(ch) || isSpecial(ch) || ch == '"') {
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
	case sMlComment:
		start = ++pos;
		if (ch == '*' && pos < src.size() && src[pos] == '/') {
			pos++;
			state = sStart;
		}
		break;
	case sPreproc:
		start = ++pos;
		if (ch == '\\') {
			state = sPreprocExtend;
		} else if (ch == '\n') {
			state = sStart;
		}
		break;
	case sPreprocExtend:
		start = ++pos;
		if (ch == '\n')
			state = sPreproc;
		else if (ch == '\r')
			;
		else
			state = sPreproc;
		break;
	case sDone:
		// pos--;
		break;
	}
}

