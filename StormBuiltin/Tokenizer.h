#pragma once
#include "Utils/Path.h"

namespace stormbuiltin {

	/**
	 * Tokenizer designed to properly tokenize the contents of .bnf files.
	 * Handles strings and basic operators (including ()[] and {}).
	 * Also handles comments. Comments have the form // ... \n
	 */
	class Tokenizer : NoCopy {
	public:
		// Tokenize data in 'src' from 'start', assuming the content comes from 'path'.
		// Note that 'src' is assumed to live at least as long as this object!
		Tokenizer(const Path &file, const String &src, nat start);

		// Get the next token in the stream. Throws an exception if the end of stream
		// has been reached.
		String next();

		// Peek.
		String peek();

		// More tokens to get?
		bool more() const;

	private:
		// Source string.
		const String &src;

		// Source file.
		const Path &file;

		// Current position.
		nat pos;

		// Different states of the tokenizer.
		enum State {
			sStart,
			sText,
			sOperator,
			sString,
			sComment,
			sMlComment,
			sDone,
		};

		// The next found token.
		String nextToken;

		// Find the next token.
		String findNext();

		// Do one step in the state-machine.
		void processChar(nat &start, State &state);
	};

}
