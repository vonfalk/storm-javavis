#pragma once
#include "OS/Types.h"

namespace storm {

	class Engine;
	class Type;

	typedef os::Int Int;
	typedef os::Nat Nat;
	typedef os::Byte Byte;
	typedef os::Float Float;
	typedef bool Bool;
	Type *intType(Engine &e);
	Type *natType(Engine &e);
	Type *byteType(Engine &e);
	Type *floatType(Engine &e);
	Type *boolType(Engine &to);


}
