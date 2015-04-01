#pragma once

namespace storm {

	class Engine;
	class Type;

	typedef code::Int Int;
	typedef code::Nat Nat;
	typedef code::Byte Byte;
	typedef bool Bool;
	Type *intType(Engine &e);
	Type *natType(Engine &e);
	Type *byteType(Engine &e);
	Type *boolType(Engine &to);

}
