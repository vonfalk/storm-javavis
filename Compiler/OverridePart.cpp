#include "stdafx.h"
#include "OverridePart.h"
#include "Function.h"
#include "Type.h"

namespace storm {

	// NOTE: Slightly dangerous to re-use the parameters from the function...
	OverridePart::OverridePart(Function *src) :	SimplePart(src->name, src->params), result(src->result) {}

	OverridePart::OverridePart(Type *parent, Function *src) :
		SimplePart(src->name, new (src) Array<Value>(*src->params)),
		result(src->result) {

		params->at(0) = thisPtr(parent);
	}

	Int OverridePart::matches(Named *candidate) const {
		Function *fn = as<Function>(candidate);
		if (!fn)
			return -1;

		Array<Value> *c = fn->params;
		if (c->count() != params->count())
			return -1;

		if (c->count() < 1)
			return -1;

		if (params->at(0).canStore(c->at(0))) {
			// Candidate is in a subclass wrt us.
			for (nat i = 1; i < c->count(); i++) {
				// Candidate needs to accept wider inputs than us.
				if (!c->at(i).canStore(params->at(i)))
					return -1;
			}

			// Candidate may return a narrower range of types.
			if (!fn->result.canStore(result))
				return -1;

		} else if (c->at(0).canStore(params->at(0))) {
			// Candidate is in a superclass wrt us.
			for (nat i = 1; i < c->count(); i++) {
				// We need to accept wider inputs than candidate.
				if (!params->at(i).canStore(c->at(i)))
					return -1;
			}

			// We may return a narrower range than candidate.
			if (!result.canStore(fn->result))
				return -1;

		} else {
			return -1;
		}

		// We always give a binary decision.
		return 0;
	}

}
