#include "stdafx.h"
#include "Int.h"

namespace storm {

	Type *intType(Engine &e) {
		return CREATE(IntType, e);
	}


	Type *natType(Engine &e) {
		return CREATE(NatType, e);
	}

}
