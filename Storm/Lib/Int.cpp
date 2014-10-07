#include "stdafx.h"
#include "Int.h"

namespace storm {

	Type *integerType() {
		Type *t = new Type(L"Int", typeValue);

		return t;
	}

}
