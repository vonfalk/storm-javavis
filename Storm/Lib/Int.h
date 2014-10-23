#pragma once
#include "Type.h"
#include "Code/Code.h"

namespace storm {

	/**
	 * Definition of the integer type in the storm language.
	 */
	typedef code::Int Int;
	Type *intType(Engine &e);


	/**
	 * Natural number (uint) type.
	 */
	typedef code::Nat Nat;
	Type *natType(Engine &e);

}
