#include "stdafx.h"
#include "Bool.h"
#include "Type.h"
#include "BoolDef.h"
#include "Engine.h"

namespace storm {

	Type *boolType(Engine &e) {
		Type *t = e.specialBuiltIn(specialBool);
		if (!t) {
			t = CREATE(BoolType, e);
			e.setSpecialBuiltIn(specialBool, t);
		}
		return t;
	}

}
