#include "stdafx.h"
#include "Bool.h"
#include "Type.h"
#include "BoolDef.h"

namespace storm {

	Type *boolType(Engine &e) {
		return CREATE(BoolType, e, e);
	}


}
