#pragma once
#include "Type.h"
#include "Code/Code.h"

namespace storm {
	STORM_PKG(core.lang);

	class IntType : public Type {
		STORM_CLASS;
	public:
		IntType();

		virtual bool isBuiltIn() const { return true; }
		virtual Function *destructor() { return null; }
	};


	/**
	 * Definition of the integer type in the storm language.
	 */
	typedef code::Int Int;
	Type *intType(Engine &e);


	class NatType : public Type {
		STORM_CLASS;
	public:
		NatType();

		virtual bool isBuiltIn() const { return true; }
		virtual Function *destructor() { return null; }
	};


	/**
	 * Natural number (uint) type.
	 */
	typedef code::Nat Nat;
	Type *natType(Engine &e);

}
