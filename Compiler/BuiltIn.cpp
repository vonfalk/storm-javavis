#include "stdafx.h"
#include "BuiltIn.h"
#include "Engine.h"

namespace storm {

	code::Ref builtin::ref(EnginePtr e, BuiltIn which) {
		return e.v.ref(which);
	}

}
