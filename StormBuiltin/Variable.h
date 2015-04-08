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

	// Read a variable.
	static Variable read(const CppScope &scope, Tokenizer &tok);
};
