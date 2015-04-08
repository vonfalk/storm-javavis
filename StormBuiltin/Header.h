#pragma once
#include "Utils/Path.h"
#include "Type.h"
#include "Function.h"
#include "Thread.h"
#include "Tokenizer.h"
#include "Variable.h"

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

	// Get variables in here.
	const vector<Variable> &getVariables();

	// Get the threads in here.
	const vector<Thread> &getThreads();

protected:
	virtual void output(wostream &o) const;

private:
	// Parsed?
	bool parsed;

	// Cache of types.
	vector<Type> types;

	// Cache of functions.
	vector<Function> functions;

	// Cache of variables.
	vector<Variable> variables;

	// Cache of threads.
	vector<Thread> threads;

	// Parse the file.
	void parse();
	void parse(Tokenizer &tok);

};
