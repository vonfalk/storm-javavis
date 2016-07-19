#include "stdafx.h"
#include "Debug.h"

namespace storm {
	namespace debug {

		DtorClass::DtorClass(int v) : value(v) {}

		DtorClass::~DtorClass() {
			PVAR(value);
		}

	}
}

