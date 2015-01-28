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

	// Const function?
	bool isConst;

	// Read a function.
	static Function read(const String &package, const CppScope &scope, const CppType &result, Tokenizer &tok);

	// Create a destructor for a type.
	static Function dtor(const String &package, const CppScope &scope);

protected:
	virtual void output(wostream &to) const;
};

