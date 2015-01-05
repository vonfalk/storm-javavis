#include "stdafx.h"
#include "Auto.h"
#include "Exception.h"

namespace storm {

	void throwTypeError(const String &context, Type *expected, Type *got) {
		throw InternalTypeError(context, expected, got);
	}

}
