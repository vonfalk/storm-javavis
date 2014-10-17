#include "stdafx.h"
#include "TypeClass.h"

namespace storm {

	Type *typeType() {
		return new Type(L"Type", typeClass);
	}

}
