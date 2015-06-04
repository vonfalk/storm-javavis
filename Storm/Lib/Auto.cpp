#include "stdafx.h"
#include "Shared/Auto.h"
#include "Exception.h"

namespace storm {

	void throwTypeError(const String &context, Type *expected, Type *got) {
		throw InternalTypeError(context, expected, got);
	}

}
