#pragma once
#include "CppName.h"
#include "TypeRef.h"
#include "SrcPos.h"

class World;

class Variable {
public:
	Variable(const CppName &name, Auto<TypeRef> type, const SrcPos &pos);

	// Position.
	SrcPos pos;

	// Name.
	CppName name;

	// Type.
	Auto<TypeRef> type;

	// Resolve types.
	void resolveTypes(World &in, const CppName &context);
};

wostream &operator <<(wostream &to, const Variable &v);

