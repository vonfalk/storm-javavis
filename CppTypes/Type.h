#pragma once
#include "SrcPos.h"
#include "CppName.h"
#include "CppType.h"

/**
 * Describes a type in C++.
 */
class Type {
public:
	// Create a type with name X, where X is the fully qualified name of the type (eg. Foo::Bar::Baz).
	Type(const CppName &name, const SrcPos &pos);


	// Name of this type.
	CppName name;

	// Parent class (if any).
	CppName parent;

	// Position.
	SrcPos pos;
};
