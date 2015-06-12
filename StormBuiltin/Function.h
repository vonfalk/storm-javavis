#pragma once

#include "CppName.h"
#include "Tokenizer.h"
#include "Type.h"
#include "Utils/Bitmask.h"

enum FnFlags {
	fnNone = 0x00,

	// Function declared as const? (member functions only).
	fnConst = 0x01,

	// Hidden 'EnginePtr' parameter?
	fnEngine = 0x02,

	// Virtual function?
	fnVirtual = 0x04,

	// External function (ie, not to be exported?)
	fnExternal = 0x10,

	// Setter function (can be used like bar.foo = 3)
	fnSetter = 0x20,
};

BITMASK_OPERATORS(FnFlags);

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

	// Flags
	FnFlags flags;

	// Read a function.
	static Function read(FnFlags flags, const String &package,
						const CppScope &scope, const CppType &result, Tokenizer &tok);

	// Create a destructor for a type.
	static Function dtor(const String &package, const CppScope &scope, bool external);

	// Create a copy-ctor for a type.
	static Function copyCtor(const String &package, const CppScope &scope, bool external);

	// Create the assignment operator for a type.
	static Function assignment(const String &package, const CppScope &scope, bool external);

protected:
	virtual void output(wostream &to) const;
};

