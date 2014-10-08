#include "stdafx.h"
#include "Str.h"
#include "Type.h"

namespace storm {


	// Create the string type.
	Type *strType() {
		return new Type(L"Str", typeClass);
	}

}
