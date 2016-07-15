#pragma once
#include "SrcPos.h"
#include "CppName.h"
#include "TypeRef.h"
#include "Variable.h"
#include "Namespace.h"

class World;

/**
 * Describes a type in C++.
 */
class Type : public Refcount {
public:
	// Create a type with name X, where X is the fully qualified name of the type (eg. std::string, Foo:Bar::Baz).
	Type(const CppName &name, const SrcPos &pos);

	// The ID of this type. Set during world.prepare().
	nat id;

	// Name of this type.
	CppName name;

	// Position of this type.
	SrcPos pos;

	// Print.
	virtual void print(wostream &to) const = 0;

	// Resolve types in here.
	virtual void resolveTypes(World &world) = 0;

	// Compute the size of this type.
	virtual Size size() const = 0;

	// Is this type heap-allocated?
	virtual bool heapAlloc() const = 0;

	// Compute pointer offsets into this type.
	vector<Offset> ptrOffsets() const;
	virtual void ptrOffsets(vector<Offset> &append) const = 0;
};

wostream &operator <<(wostream &to, const Type &type);

/**
 * Describes a class or struct from C++.
 */
class Class : public Type, public Namespace {
public:
	// Create a type with name X, where X is the fully qualified name of the type (eg. Foo::Bar::Baz).
	Class(const CppName &name, const SrcPos &pos, bool valueType);

	// Is this a value-type?
	bool valueType;

	// Parent class (if any).
	CppName parent;

	// Actual type of the parent class.
	Type *parentType;

	// Member variables (non-static). All have their name relative to the enclosing type.
	vector<Variable> variables;

	// Add a variable.
	void add(const Variable &v);

	// Resolve types in here.
	virtual void resolveTypes(World &world);

	// Compute our size.
	virtual Size size() const;

	// Compute our pointer offsets.
	virtual void ptrOffsets(vector<Offset> &append) const;

	// Print ourselves.
	virtual void print(wostream &to) const;

	// Is this type heap-allocated?
	virtual bool heapAlloc() const { return !valueType; }

};

/**
 * Describe an enumeration type from C++.
 */
class Enum : public Type {
public:
	// Create.
	Enum(const CppName &name, const SrcPos &pos);

	// Resolve types.
	virtual void resolveTypes(World &world);

	// Compute size.
	virtual Size size() const;

	// Generate pointer offsets.
	virtual void ptrOffsets(vector<Offset> &append) const;

	// Print.
	virtual void print(wostream &to) const;

	// Never allocated on heap.
	virtual bool heapAlloc() const { return false; }
};

