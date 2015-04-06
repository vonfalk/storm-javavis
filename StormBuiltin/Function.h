#pragma once

#include "CppName.h"
#include "Tokenizer.h"
#include "Type.h"

/**
 * Describes a single function that is to be exported.
 */
class Function : public Printable {
public:

	// Name of the cpp function.
	CppScope cppScope;

	// Name of the storm function.
	String name;

	// Storm package.
	String package;

	// Return type
	CppType result;

	// Parameter types.
	vector<CppType> params;

	// On thread?
	CppName thread;

	// Const function?
	bool isConst;

	// Has a hidden 'Engine &' parameter?
	bool engineFn;

	// Read a function.
	static Function read(bool engineFn, const String &package, const CppScope &scope, const CppType &result, Tokenizer &tok);

	// Create a destructor for a type.
	static Function dtor(const String &package, const CppScope &scope);

	// Create a copy-ctor for a type.
	static Function copyCtor(const String &package, const CppScope &scope);

	// Create the assignment operator for a type.
	static Function assignment(const String &package, const CppScope &scope);

protected:
	virtual void output(wostream &to) const;
};

