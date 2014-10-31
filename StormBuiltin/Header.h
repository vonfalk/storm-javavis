#pragma once
#include "Utils/Path.h"
#include "Type.h"
#include "Function.h"
#include "Tokenizer.h"

/**
 * Represents a found header.
 */
class Header : public Printable, NoCopy {
public:
	// Create the object.
	Header(const Path &file);

	// Header file.
	const Path file;

	// Get the contents of this header:
	const vector<Type> &getTypes();

	// Get the functions in here.
	const vector<Function> &getFunctions();

protected:
	virtual void output(wostream &o) const;

private:
	// Parsed?
	bool parsed;

	// Cache of types.
	vector<Type> types;

	// Cache of functions.
	vector<Function> functions;

	// Parse the file.
	void parse();
	void parse(Tokenizer &tok);

};
