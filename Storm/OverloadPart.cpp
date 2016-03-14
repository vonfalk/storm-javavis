#include "stdafx.h"
#include "OverloadPart.h"
#include "Function.h"
#include "Type.h"

namespace storm {

	OverloadPart::OverloadPart(Par<Function> fn) : SimplePart(fn->name, fn->params) {}

	Int OverloadPart::matches(Par<Named> candidate) {
		const vector<Value> &c = candidate->params;
		if (c.size() != data.size())
			return -1;

		int distance = 0;
		// Check the rest...
		for (nat i = 1; i < c.size(); i++) {
			if (!c[i].matches(data[i], candidate->flags))
				return -1;
			if (data[i].type)
				distance += data[i].type->distanceFrom(c[i].type);
		}
		return distance;
	}

}
