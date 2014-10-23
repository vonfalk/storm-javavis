#include "stdafx.h"
#include "TypeClass.h"

namespace storm {

	Type *typeType(Engine &e) {
		return new Type(e, L"Type", typeClass);
	}

}
