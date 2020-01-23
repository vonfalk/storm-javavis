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

	// Flags.
	enum Flags {
		// Is this a member function?
		isMember = 0x0001,

		// Is this a static function?
		isStatic = 0x0002,

		// Trailing const modifier?
		isConst = 0x0004,

		// Leading 'virtual' modifier? Means it is not final.
		isVirtual = 0x0008,

		// Is this function marked 'abstract'?
		isAbstract = 0x0010,

		// Assignment function (marked STORM_ASSIGN).
		isAssign = 0x0020,

		// This is an assignment function that should be wrapped.
		wrapAssign = 0x0040,

		// Usable for casting?
		castMember = 0x0080,

		// Exported?
		// Most non-exported functions are not present in the World object. Some non-exported
		// functions (such as abstract functions) do, however, need to appear there.
		exported = 0x0100,
	};

	// Flags.
	nat flags;

	// Flags manipulation.
	inline bool has(Flags flag) const { return (flags & flag) == flag; }
	inline void set(Flags flag) { flags |= flag; }
	inline void clear(Flags flag) { flags &= ~nat(flag); }
	inline void set(Flags flag, bool value) {
		if (value)
			set(flag);
		else
			clear(flag);
	}

	// Run on a specific thread?
	CppName thread;

	// Resolved type of the thread.
	Thread *threadType;

	// Resolve types.
	void resolveTypes(World &w, const CppName &ctx);
};

wostream &operator <<(wostream &to, const Function &fn);
