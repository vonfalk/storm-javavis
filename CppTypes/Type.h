#pragma once
#include "SrcPos.h"
#include "CppName.h"
#include "CppType.h"
#include "Variable.h"
#include "Namespace.h"

class World;

/**
 * Describes a type in C++.
 */
class Type : public Namespace {
public:
	// Create a type with name X, where X is the fully qualified name of the type (eg. Foo::Bar::Baz).
	Type(const CppName &name, const SrcPos &pos, bool valueType);

	// The ID of this type. Set during world.prepare().
	nat id;

	// Is this a value-type?
	bool valueType;

	// Name of this type.
	CppName name;

	// Parent class (if any).
	CppName parent;

	// Actual type of the parent class.
	Type *parentType;

	// Position.
	SrcPos pos;

	// Member variables (non-static). All have their name relative to the enclosing type.
	vector<Variable> variables;

	// Add a variable.
	void add(const Variable &v);

	// Resolve types in here.
	void resolveTypes(World &world);

	// Compute our size.
	Size size() const;

	// Compute our pointer offsets.
	vector<Offset> ptrOffsets() const;
};

wostream &operator <<(wostream &to, const Type &type);
