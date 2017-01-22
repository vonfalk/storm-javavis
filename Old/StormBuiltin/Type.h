#pragma once
#include "CppName.h"

/**
 * Describes an exported class.
 */
class Type : public Printable {
public:
	Type() {}

	// Built-in type.
	Type(const String &name, const String &pkg, bool value = false)
		: name(name), super(), package(pkg), cppName(vector<String>(1,name)),
		  primitive(true), external(true), value(value) {}

	Type(const String &name, const CppSuper &super, const String &pkg, const CppName &cppName,
		bool value = false, bool external = false)
		: name(name), super(super), package(pkg), cppName(cppName),
		  primitive(false), external(external), value(value) {}

	// Name of the class.
	String name;

	// Superclass (if any, empty otherwise).
	CppSuper super;

	// Package.
	String package;

	// C++-name.
	CppName cppName;

	// Primitive (built-in) type?
	bool primitive;

	// Value type?
	bool value;

	// External type?
	bool external;

	// Concat 'name' and 'package'.
	String fullName() const;

protected:
	virtual void output(wostream &to) const;

};


/**
 * Keep a number of types so that we can generate
 * fully qualified names for types, given their scope.
 */
class Types : public Printable {
public:
	explicit Types(bool forCompiler);

	// Add a type.
	void add(const Type &type);

	// Find a type. scope.parent() is used for lookup.
	Type find(const CppName &name, const CppName &scope) const;

	// Get all types in here.
	const vector<Type> &getTypes() const;

	// Get a type:s id (based on its name).
	nat typeId(const CppName &name) const;

	// Special case when we're giving types to the compiler.
	bool forCompiler;

	// Using namespaces globally?
	vector<CppName> usedNamespaces;

protected:
	virtual void output(wostream &to) const;

private:
	// Known types.
	typedef map<CppName, Type> T;
	T types;

	// Order of types (initialized when calling 'getTypes').
	typedef map<CppName, nat> Tid;
	mutable Tid typeIds;

	// Cached types.
	mutable vector<Type> ordered;

	// Cached data valid?
	mutable bool cacheValid;

	// Invalidate any cached data.
	void invalidate() const;

	// Create cached data.
	void create() const;
};
