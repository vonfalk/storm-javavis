#include "stdafx.h"
#include "TypeLayout.h"

namespace storm {

	void TypeLayout::add(NameOverload *n) {
		if (TypeVar *v = as<TypeVar>(n))
			add(v);
	}

	void TypeLayout::add(TypeVar *v) {
		// Ok, we need to find some space somewhere!
	}

}
