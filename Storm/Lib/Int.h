#pragma once
#include "Type.h"
#include "Code/Code.h"

namespace storm {

	class IntType : public Type {
		STORM_CLASS;
	public:
		IntType(Engine &e) : Type(e, L"Int", typeValue) {}

		virtual nat size() const {
			return sizeof(code::Int);
		}
	};


	/**
	 * Definition of the integer type in the storm language.
	 */
	typedef code::Int Int;
	Type *intType(Engine &e);


	class NatType : public Type {
		STORM_CLASS;
	public:
		NatType(Engine &e) : Type(e, L"Nat", typeValue) {}

		virtual nat size() const {
			return sizeof(code::Nat);
		}
	};


	/**
	 * Natural number (uint) type.
	 */
	typedef code::Nat Nat;
	Type *natType(Engine &e);

}
