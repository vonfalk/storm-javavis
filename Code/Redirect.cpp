#include "stdafx.h"
#include "Redirect.h"
#include "Reference.h"
#include "Instruction.h"

namespace code {

	Redirect::Redirect() : resultSize(0), resultBuiltIn(false) {}

	void Redirect::result(Size size, bool i) {
		resultSize = size;
		resultBuiltIn = i;
	}

	void Redirect::param(Size size, Value dtor, bool ptr) {
		Param p = { size, dtor, ptr };
		params.push_back(p);
	}

	Listing Redirect::code(const Value &fn, const Value &param) {
		Listing l;
		// TODO: This is not completely accurate since the this-ptr are always before
		// the result parameter. In this case, the this ptr should not have a dtor, nor should
		// the parameter, so this will probably work in many cases.

		if (!resultBuiltIn) {
			l.frame.createParameter(resultSize, false); // no dtor needed here.
		}

		for (nat i = 0; i < params.size(); i++) {
			Param &p = params[i];
			FreeOpt o = freeOnException;
			if (p.byPtr)
				o |= freePtr;
			l.frame.createParameter(p.size, false, p.dtor, o);
		}

		l << prolog();

		if (param != Value())
			l << fnParam(param);
		l << fnCall(fn, Size::sPtr);

		l << epilog(); // preserves ptrA
		l << jmp(ptrA);

		return l;
	}
}
