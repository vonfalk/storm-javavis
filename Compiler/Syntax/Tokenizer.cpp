#include "stdafx.h"
#include "Tokenizer.h"
#include "Compiler/Exception.h"
#include "Core/StrBuf.h"

namespace storm {
	namespace syntax {

		Token::Token(Str *str, Nat len, const SrcPos &pos) : pos(pos), str(str), len(len) {}

		bool Token::empty() const {
			return len == 0;
		}

		const wchar *Token::start() const {
			return str->c_str() + pos.pos;
		}

		bool Token::operator ==(const wchar *t) const {
			const wchar *s = start();
			for (nat i = 0; i < len; i++) {
				if (s[i] == 0)
					return false;
				if (s[i] != t[i])
					return false;
			}
			return true;
		}

		bool Token::isStrLiteral() const {
			const wchar *s = start();
			if (*s != '"')
				return false;
			return s[len-1] == '"';
		}

		Str *Token::strLiteral() const {
			StrBuf *to = new (str) StrBuf();
			if (!isStrLiteral())
				return to->toS();

			// TODO: Proper escape/unescape.
			const wchar *s = start();
			for (nat i = 1; i < len - 1; i++) {
				if (s[i] == '\\') {
					if (i + 1 >= len - 1) {
						to->addRaw('\\');
					} else {
						i++;
						switch (s[i]) {
						case 'n':
							to->addRaw('\n');
							break;
						case 'r':
							to->addRaw('\r');
							break;
						case 't':
							to->addRaw('\t');
							break;
						case 'v':
							to->addRaw('\v');
							break;
						case '"':
							to->addRaw('\"');
							break;
						default:
							to->addRaw('\\');
							to->addRaw(s[i]);
							break;
						}
					}
				} else {
					to->addRaw(s[i]);
				}
			}

			return to->toS();
		}

		Str *Token::toS() const {
			const wchar *start = this->start();
			const wchar *end = start + len;
			return new (str) Str(start, end);
		}

		wostream &operator <<(wostream &to, const Token &t) {
			return to << t.toS()->c_str();
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

		Tokenizer::Tokenizer(Url *path, Str *src, Nat start)
			: src(src),
			  file(path),
			  pos(start),
			  lookahead(src, 0, SrcPos(path, 0)) {

			lookahead = findNext();
		}

		Token Tokenizer::next() {
			Token t = peek();
			skip();
			return t;
		}

		Token Tokenizer::peek() const {
			if (!more())
				throw SyntaxError(position(), L"Unexpected end of file.");
			return lookahead;
		}

		void Tokenizer::skip() {
			lookahead = findNext();
		}

		bool Tokenizer::skipIf(const wchar *token) {
			if (!more())
				throw SyntaxError(position(), L"Unexpected end of file.");

			if (lookahead == token) {
				skip();
				return true;
			} else {
				return false;
			}
		}

		void Tokenizer::expect(const wchar *token) {
			if (!more())
				throw SyntaxError(position(), L"Unexpected end of file.");

			if (lookahead != token)
				throw SyntaxError(lookahead.pos, L"Expected " + ::toS(token) + L" but got " + ::toS(lookahead.toS()));

			skip();
		}

		bool Tokenizer::more() const {
			return !lookahead.empty();
		}

		SrcPos Tokenizer::position() const {
			return SrcPos(file, pos);
		}

		Token Tokenizer::findNext() {
			State state = sStart;
			nat start = pos;

			while (state != sDone)
				processChar(start, state);

			return Token(src, pos - start, SrcPos(file, start));
		}

		void Tokenizer::processChar(Nat &start, State &state) {
			const wchar *str = src->c_str();
			wchar ch = str[pos];
			if (ch == 0) {
				state = sDone;
				return;
			}

			if (ch == '/' && str[pos+1] == '/') {
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
			case sDone:
				if (pos > 0)
					pos--;
				break;
			}
		}

	}
}
