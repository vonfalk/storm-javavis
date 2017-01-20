#pragma once
#include "SrcPos.h"
#include "TypeRef.h"
#include "CppName.h"
#include "Thread.h"
#include "Access.h"

/**
 * A function declared in C++.
 */
class Function {
public:
	Function(const CppName &name, const String &pkg, Access access, const SrcPos &pos, Auto<TypeRef> result);

	// String constants for constructor and destructor names.
	static const String ctor, dtor;

	// Name (__init and __destroy are constructor and destructor).
	CppName name;

	// Package.
	String pkg;

	// Declared at?
	SrcPos pos;

	// Access.
	Access access;

	// Parameters (any implicit this-pointer is not present here).
	vector<Auto<TypeRef>> params;

	// Return type.
	Auto<TypeRef> result;

	// Is this a member function?
	bool isMember;

	// Trailing const modifier?
	bool isConst;

	// Leading 'virtual' modifier?
	bool isVirtual;

	// This is an assignment function that should be wrapped.
	bool wrapAssign;

	// Usable for casting?
	bool castMember;

	// Run on a specific thread?
	CppName thread;

	// Resolved type of the thread.
	Thread *threadType;

	// Resolve types.
	void resolveTypes(World &w, CppName &ctx);
};

wostream &operator <<(wostream &to, const Function &fn);
