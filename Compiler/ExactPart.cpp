#include "stdafx.h"
#include "ExactPart.h"
#include "Named.h"

namespace storm {

	ExactPart::ExactPart(Str *name, Array<Value> *params) : SimplePart(name, params) {}

	Int ExactPart::matches(Named *candidate, Scope scope) const {
		if (candidate->params->count() != params->count())
			return -1;

		for (Nat i = 0; i < params->count(); i++)
			if (params->at(i) != candidate->params->at(i))
				return -1;

		return 0;
	}

}
