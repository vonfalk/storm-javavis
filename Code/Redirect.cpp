#include "stdafx.h"
#include "Redirect.h"
#include "Reference.h"
#include "Instruction.h"

namespace code {

	Redirect::Redirect() {}

	void Redirect::param(nat size, Ref dtor) {
		Param p = { size, dtor };
		params.push_back(p);
	}

	Listing Redirect::code(const Value &fn, const Value &param) {
		Listing l;
		Block root = l.frame.root();

		for (nat i = 0; i < params.size(); i++) {
			Param &p = params[i];
			l.frame.createParameter(p.size, false, p.dtor);
		}

		l << prolog();

		l << fnParam(param);
		l << fnCall(fn, 0); // (sizeof void *)

		l << epilog(); // preserves ptrA
		l << jmp(ptrA);

		return l;
	}
}
