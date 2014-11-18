#pragma once
#include "Utils/Path.h"

/**
 * Tokenizer designed to properly tokenize the contents of .bnf files.
 * Handles strings and basic operators (including ()[] and {}).
 * Also handles comments. Comments have the form // ... \n
 * NOTE TO SELF: / * ... * / leaves a / as an artifact! This does not break anything,
 * and is therefore not fixed yet.
 */
class Tokenizer : NoCopy {
public:
	// Tokenize data in 'src' from 'start', assuming the content comes from 'path'.
	// Note that 'src' is assumed to live at least as long as this object!
	Tokenizer(const String &src, nat start);

	// Get the next token in the stream. Throws an exception if the end of stream
	// has been reached.
	String next();

	// Peek.
	String peek();

	// More tokens to get?
	bool more() const;

	// Get a token and see it is the correct one.
	void expect(const String &s);

private:
	// Source string.
	const String &src;

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

