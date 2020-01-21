#pragma once
#include "SrcPos.h"
#include "TypeRef.h"
#include "CppName.h"
#include "Thread.h"
#include "Access.h"
#include "Doc.h"

/**
 * A function declared in C++.
 */
class Function {
public:
	Function(const CppName &name, const String &pkg, Access access,
			const SrcPos &pos, const Auto<Doc> &doc, Auto<TypeRef> result);

	// String constants for constructor and destructor names.
	static const String ctor, dtor;

	// Name (__init and __destroy are constructor and destructor).
	CppName name;

	// Package.
	String pkg;

	// Name in Storm.
	String stormName;

	// Declared at?
	SrcPos pos;

	// Documentation.
	Auto<Doc> doc;

	// Access.
	Access access;

	// Parameters (any implicit this-pointer is not present here).
	vector<Auto<TypeRef>> params;

	// Parameter names.
	vector<String> paramNames;

	// Return type.
	Auto<TypeRef> result;

	// Is this a member function?
	bool isMember;

	// Is this a static function?
	bool isStatic;

	// Trailing const modifier?
	bool isConst;

	// Leading 'virtual' modifier? Means it is not final.
	bool isVirtual;

	// Is this function marked 'abstract'?
	bool isAbstract;

	// Assignment function (marked STORM_ASSIGN).
	bool isAssign;

	// This is an assignment function that should be wrapped.
	bool wrapAssign;

	// Usable for casting?
	bool castMember;

	// Exported?
	// Most non-exported functions are not present in the World object. Some non-exported
	// functions (such as abstract functions) do, however, need to appear there.
	bool exported;

	// Run on a specific thread?
	CppName thread;

	// Resolved type of the thread.
	Thread *threadType;

	// Resolve types.
	void resolveTypes(World &w, const CppName &ctx);
};

wostream &operator <<(wostream &to, const Function &fn);
