#pragma once

namespace storm {

	class Engine;
	class Type;

	typedef code::Int Int;
	typedef code::Nat Nat;
	typedef bool Bool;
	Type *intType(Engine &e);
	Type *natType(Engine &e);
	Type *boolType(Engine &to);

}
