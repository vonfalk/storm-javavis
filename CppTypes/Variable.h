#pragma once
#include "CppName.h"
#include "CppType.h"
#include "SrcPos.h"

class World;

class Variable {
public:
	Variable(const CppName &name, Auto<CppType> type, const SrcPos &pos);

	// Position.
	SrcPos pos;

	// Name.
	CppName name;

	// Type.
	Auto<CppType> type;

	// Resolve types.
	void resolveTypes(World &in);
};

wostream &operator <<(wostream &to, const Variable &v);

