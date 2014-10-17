#include "stdafx.h"
#include "Str.h"
#include "Type.h"

namespace storm {

	nat Str::count() const {
		return v.size();
	}


	// Create the string type.
	Type *strType() {
		return new Type(L"Str", typeClass);
	}

}
