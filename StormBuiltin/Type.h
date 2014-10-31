#pragma once
#include "CppName.h"

/**
 * Describes an exported class.
 */
class Type : public Printable {
public:
	// Built-in type.
	Type(const String &name, const String &pkg)
		: name(name), super(), package(pkg), cppName(vector<String>(1,name)), exported(false) {}

	Type(const String &name, const CppName &super, const String &pkg, const CppName &cppName)
		: name(name), super(super), package(pkg), cppName(cppName), exported(true) {}

	// Name of the class.
	String name;

	// Superclass (if any, empty otherwise).
	CppName super;

	// Package.
	String package;

	// C++-name.
	CppName cppName;

	// Export this type?
	bool exported;

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

	// Add a type.
	void add(const Type &type);

	// Find a type. scope.parent() is used for lookup.
	Type find(const CppName &name, const CppName &scope) const;

	// Get all types in here.
	vector<Type> getTypes() const;

protected:
	virtual void output(wostream &to) const;

private:
	// Known types.
	typedef map<CppName, Type> T;
	T types;
};
