#pragma once
#include "CppName.h"
#include "Tokenizer.h"

class Variable {
public:
	// Scope.
	CppScope cppScope;

	// Name.
	String name;

	// Type.
	CppType type;

	// External?
	bool external;

	// Read a variable.
	static Variable read(bool external, const CppScope &scope, Tokenizer &tok);
};
