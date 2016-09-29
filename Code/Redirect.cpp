#include "stdafx.h"
#include "Redirect.h"

namespace code {

	RedirectParam::RedirectParam(ValType val) : val(val) {}

	RedirectParam::RedirectParam(ValType val, Operand freeFn, Bool byPtr) : val(val), freeFn(freeFn), byPtr(byPtr) {}

	Listing *redirect(Array<RedirectParam> *params, Operand fn, Operand param) {
		Listing *l = new (params) Listing();

		for (nat i = 0; i < params->count(); i++) {
			RedirectParam &p = params->at(i);
			FreeOpt o = freeOnException;
			if (p.byPtr)
				o |= freePtr;
			l->createParam(p.val, p.freeFn, o);
		}

		*l << prolog();

		if (!param.empty())
			*l << fnParam(param);
		*l << fnCall(fn, valPtr());

		*l << epilog(); // Preserves ptrA.
		*l << jmp(ptrA);

		return l;
	}

}
