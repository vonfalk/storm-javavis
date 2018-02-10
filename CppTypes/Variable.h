#pragma once
#include "CppName.h"
#include "TypeRef.h"
#include "SrcPos.h"
#include "Access.h"
#include "Doc.h"

class World;

class Variable {
public:
	Variable(const CppName &name, Auto<TypeRef> type, Access access, const SrcPos &pos, const Auto<Doc> &doc);

	// Name.
	CppName name;

	// Name in Storm.
	String stormName;

	// Type.
	Auto<TypeRef> type;

	// Position.
	SrcPos pos;

	// Documentation.
	Auto<Doc> doc;

	// Access.
	Access access;

	// Resolve types.
	void resolveTypes(World &in, const CppName &context);
};

wostream &operator <<(wostream &to, const Variable &v);

