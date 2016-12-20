#pragma once
#include "SrcPos.h"
#include "CppName.h"
#include "TypeRef.h"
#include "Variable.h"
#include "Function.h"
#include "Namespace.h"
#include "Thread.h"

class World;

struct ScannedVar {
	CppName typeName;
	String varName;
};

/**
 * Describes a type in C++.
 */
class Type : public Refcount {
public:
	// Create a type with name X, where X is the fully qualified name of the type (eg. std::string, Foo:Bar::Baz).
	Type(const CppName &name, const String &pkg, const SrcPos &pos);

	// The ID of this type. Set during world.prepare().
	nat id;

	// Name of this type.
	CppName name;

	// Package.
	String pkg;

	// Position of this type.
	SrcPos pos;

	// Print.
	virtual void print(wostream &to) const = 0;

	// Resolve types in here.
	virtual void resolveTypes(World &world) = 0;

	// Compute the size of this type.
	virtual Size size() const = 0;

	// Is this type heap-allocated? (value types return false, class types return true).
	virtual bool heapAlloc() const = 0;

	// Compute pointer offsets into this type.
	vector<Offset> ptrOffsets() const;
	virtual void ptrOffsets(vector<Offset> &append) const = 0;

	// Get all members who need to be scanned (same set as in 'ptrOffsets').
	vector<ScannedVar> scannedVars() const;
	virtual void scannedVars(vector<ScannedVar> &append) const = 0;
};

wostream &operator <<(wostream &to, const Type &type);

/**
 * Describes a class or struct from C++.
 */
class Class : public Type {
public:
	// Create a type with name X, where X is the fully qualified name of the type (eg. Foo::Bar::Baz).
	Class(const CppName &name, const String &pkg, const SrcPos &pos);

	// Is this a value-type?
	bool valueType;

	// Parent class (if any).
	CppName parent;

	// Hidden parent?
	bool hiddenParent;

	// Have we found a destructor?
	bool dtorFound;

	// Actual type of the parent class.
	Type *parentType;

	// Which thread is this type associated to?
	CppName thread;

	// The thread object we're associated with (only for the classes directly inheriting from ObjectOn<T>).
	Thread *threadType;

	// Is this an actor? Only usable after resolveTypes() have been called.
	bool isActor() const;

	// Member variables (non-static). All have their name relative to the enclosing type.
	vector<Variable> variables;

	// Does this type have a declared destructor?
	bool hasDtor() const;

	// Add a variable.
	void add(const Variable &v);

	// Add a function.
	void add(const Function &f);

	// Resolve types in here.
	virtual void resolveTypes(World &world);

	// Compute our size.
	virtual Size size() const;

	// Compute our pointer offsets.
	virtual void ptrOffsets(vector<Offset> &append) const;
	virtual void scannedVars(vector<ScannedVar> &append) const;

	// External offset computation. First call 'baseOffset' to initialize the 'size' variable. Then
	// call 'varOffset' for 'var' = 0, 1, .... 'varOffset' returns the offset of variable 'var' and
	// updates the size variable.
	Size baseOffset() const;
	Offset varOffset(nat var, Size &offset) const;

	// Print ourselves.
	virtual void print(wostream &to) const;

	// Is this type heap-allocated?
	virtual bool heapAlloc() const { return !valueType; }

};

/**
 * Namespace for a class.
 */
class ClassNamespace : public Namespace {
public:
	ClassNamespace(World &world, Class &owner);

	virtual void add(const Variable &v);
	virtual void add(const Function &f);

private:
	World &world;
	Class &owner;
};

/**
 * Describe a primitive type in Storm.
 */
class Primitive : public Type {
public:
	// Create.
	Primitive(const CppName &name, const String &pkg, const CppName &generate, const SrcPos &pos);

	// Function used to generate this primitive.
	CppName generate;

	// Print.
	virtual void print(wostream &to) const;

	// Resolve types in here.
	virtual void resolveTypes(World &world);

	// Compute the size of this type.
	virtual Size size() const;

	// Is this type heap-allocated?
	virtual bool heapAlloc() const;

	// Compute pointer offsets into this type.
	virtual void ptrOffsets(vector<Offset> &append) const;
	virtual void scannedVars(vector<ScannedVar> &append) const;

private:
	// Size of this type.
	Size mySize;
};

/**
 * Describe an enumeration type from C++.
 */
class Enum : public Type {
public:
	// Create.
	Enum(const CppName &name, const String &pkg, const SrcPos &pos);

	// Members in the enum (not their values, we can easily generate code that fetches those for us).
	vector<String> members;

	// Is this enum used as a bitmask?
	bool bitmask;

	// Resolve types.
	virtual void resolveTypes(World &world);

	// Compute size.
	virtual Size size() const;

	// Generate pointer offsets.
	virtual void ptrOffsets(vector<Offset> &append) const;
	virtual void scannedVars(vector<ScannedVar> &append) const;

	// Print.
	virtual void print(wostream &to) const;

	// Never allocated on heap.
	virtual bool heapAlloc() const { return false; }
};

/**
 * Describe a template type from C++ (declared using STORM_TEMPLATE(x))
 */
class Template : public Refcount {
public:
	// Create.
	Template(const CppName &name, const String &pkg, const CppName &generator, const SrcPos &pos);

	// Our id. Set when added to the world.
	nat id;

	// Name.
	CppName name;

	// Package.
	String pkg;

	// Function generating Storm types (only in the compiler right now).
	CppName generator;

	// Position.
	SrcPos pos;
};
