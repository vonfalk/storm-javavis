#pragma once
#include "SrcPos.h"

namespace storm {

	/**
	 * Token generated by the tokenizer.
	 */
	class Token {
	public:
		inline Token(const String &token, const SrcPos &pos) : token(token), pos(pos) {}

		// Contents of this token.
		String token;

		// Where is the starting point of this token?
		SrcPos pos;

		// Compare the actual token.
		inline bool operator ==(const Token &o) const { return token == o.token; }
		inline bool operator !=(const Token &o) const { return token != o.token; }

		// Empty token?
		inline bool empty() const { return token.size() == 0; }

		// Is this a string?
		bool isStr() const;

		// Extract the string from the token. Assumes isStr().
		String strVal() const;
	};

	// Output.
	wostream &operator <<(wostream &to, const Token &t);


	/**
	 * Tokenizer designed to properly tokenize the contents of .bnf files.
	 * Handles strings and basic operators (including ()[] and {}).
	 * Also handles comments. Comments have the form // ... \n
	 */
	class Tokenizer : NoCopy {
	public:
		// Tokenize data in 'src' from 'start', assuming the content comes from 'path'.
		// Note that 'src' is assumed to live at least as long as this object!
		Tokenizer(Par<Url> path, const String &src, nat start);

		// Get the next token in the stream. Throws an exception if the end of stream
		// has been reached.
		Token next();

		// Skip one token.
		void skip();

		// Peek.
		Token peek();

		// Expect a token.
		void expect(const String &token);

		// Current position.
		inline SrcPos position() const { return SrcPos(srcFile, pos); }

		// More tokens to get?
		bool more() const;

	private:
		// Source string.
		const String &src;

		// Source file.
		Auto<Url> srcFile;

		// Current position.
		nat pos;

		// Different states of the tokenizer.
		enum State {
			sStart,
			sText,
			sOperator,
			sString,
			sComment,
			sDone,
		};

		// The next found token.
		Token nextToken;

		// Find the next token.
		Token findNext();

		// Do one step in the state-machine.
		void processChar(nat &start, State &state);
	};

}