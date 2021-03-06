#pragma once
#include "Core/Str.h"
#include "Core/SrcPos.h"

namespace storm {
	namespace syntax {

		/**
		 * Tokenizer for parsing the syntax language. This is not designed for being accessed from
		 * Storm.
		 */

		/**
		 * A token from the parser. Do not store these anywhere except on the stack.
		 */
		class Token {
		public:
			Token(Str *str, Nat len, const SrcPos &pos);

			// Contents of this token.
			SrcPos pos;

			// Empty token?
			bool empty() const;

			// Is this token equal to 'str'?
			bool operator ==(const wchar *str) const;
			bool operator !=(const wchar *str) const { return !(*this == str); }

			// Is his a string literal.
			bool isStrLiteral() const;

			// Get the string literal without quotes.
			Str *strLiteral() const;

			// Return a Str representing this token.
			Str *toS() const;

		private:
			// Contents of this token. 'start' is stored inside 'pos'.
			Str *str;
			Nat len;

			// Get the start pointer.
			const wchar *start() const;
		};

		// Output.
		wostream &operator <<(wostream &to, const Token &t);


		/**
		 * Tokenizer. Handles strings and basic operators (including ()[] and {}). Also handles
		 * comments of the form // ... \n
		 */
		class Tokenizer : NoCopy {
		public:
			// Tokenize data in 'src' from 'start', assuming the content comes from 'path'.
			Tokenizer(Url *path, Str *src, Nat start);

			// Get the next token in the stream. Throws an exception if the end of stream has been reached.
			Token next();

			// Peek the current token.
			Token peek() const;

			// Skip one token.
			void skip();

			// Skip a token if it is 'token'.
			bool skipIf(const wchar *token);

			// Expect a token.
			void expect(const wchar *token);

			// More tokens to get?
			bool more() const;

			// Current position.
			SrcPos position() const;

			// Position of the last comment block. Returns SrcPos() if none.
			SrcPos comment() const;

			// Clear the current comment.
			void clearComment();

		private:
			// Source string.
			Str *src;

			// Source file.
			Url *file;

			// Current position.
			Nat pos;

			// Start of the last comment block. Set to Nat(-1) if none.
			Nat commentStart;

			// No comment.
			static Nat invalid;

			// Different states of the tokenizer.
			enum State {
				sStart,
				sText,
				sOperator,
				sString,
				sComment,
				sDone,
			};

			// The next token.
			Token lookahead;

			// Find the next token.
			Token findNext();

			// Do one step in the state machine.
			void processChar(Nat &start, State &state, bool &firstComment);
		};

	}
}
