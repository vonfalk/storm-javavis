#include "stdafx.h"
#include "Int.h"
#include "Engine.h"

namespace storm {

	Type *intType(Engine &e) {
		Type *t = e.specialBuiltIn(specialInt);
		if (!t) {
			t = CREATE(IntType, e);
			e.setSpecialBuiltIn(specialInt, t);
		}
		return t;
	}


	Type *natType(Engine &e) {
		Type *t = e.specialBuiltIn(specialNat);
		if (!t) {
			t = CREATE(NatType, e);
			e.setSpecialBuiltIn(specialNat, t);
		}
		return t;
	}

}
