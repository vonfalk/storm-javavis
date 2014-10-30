#include "stdafx.h"
#include "Tokenizer.h"
#include "Exception.h"

static const wchar *operators = L"+?*=><.";
static const wchar *specials = L"(){},-;";

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

Tokenizer::Tokenizer(const String &src, nat start)
	: src(src), pos(start), nextToken(L"") {
	nextToken = findNext();
}

String Tokenizer::next() {
	String t = peek();
	nextToken = findNext();
	return t;
}

String Tokenizer::peek() {
	if (!more())
		throw Error(L"End of file!");

	return nextToken;
}

void Tokenizer::expect(const String &t) {
	String tok = next();
	if (tok != t)
		throw Error(L"Expected " + t + L" but got " + tok);
}

bool Tokenizer::more() const {
	return !nextToken.empty();
}

String Tokenizer::findNext() {
	State state = sStart;
	nat start = pos;

	while (state != sDone) {
		processChar(start, state);
	}

	return src.substr(start, pos - start);
}

void Tokenizer::processChar(nat &start, State &state) {
	if (pos >= src.size()) {
		state = sDone;
		return;
	}

	wchar ch = src[pos];

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
	case sMlComment:
		start = ++pos;
		if (ch == '*' && pos < src.size() && src[pos] == '/')
			state = sStart;
		break;
	case sDone:
		pos--;
		break;
	}

}

