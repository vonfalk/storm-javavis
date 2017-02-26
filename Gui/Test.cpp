#include "stdafx.h"
#include "Test.h"

namespace gui {

	Int test(Int v) {
		return v + 1;
	}

	Int test(Array<Int> *v) {
		if (runtime::typeOf(v) == Array<Int>::stormType(v->engine()))
			return 0;

		Int r = 0;
		for (Nat i = 0; i < v->count(); i++)
			r += v->at(i);
		return r;
	}
}
