#pragma once
#include "SrcPos.h"
#include "TypeRef.h"
#include "CppName.h"

/**
 * A function declared in C++.
 */
class Function {
public:
	Function(const CppName &name, const SrcPos &pos, Auto<TypeRef> result);

	// String constants for constructor and destructor names.
	static const String ctor, dtor;

	// Name (__init and __destroy are constructor and destructor).
	CppName name;

	// Declared at?
	SrcPos pos;

	// Parameters (any implicit this-pointer is not present here).
	vector<Auto<TypeRef>> params;

	// Return type.
	Auto<TypeRef> result;

	// Trailing const modifier?
	bool isConst;

	// Leading 'virtual' modifier?
	bool isVirtual;

	// Run on a specific thread?
	CppName thread;

	// Resolve types.
	void resolveTypes(World &w, CppName &ctx);
};

wostream &operator <<(wostream &to, const Function &fn);
