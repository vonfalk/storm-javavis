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

	void Redirect::addParam(code::Listing &to, const Param &p) {
		FreeOpt o = freeOnException;
		if (p.byPtr)
			o |= freePtr;
		to.frame.createParameter(p.size, false, p.dtor, o);
	}

	Listing Redirect::code(const Value &fn, bool memberFn, const Value &param) {
		Listing l;
		nat start = 0;

		if (memberFn) {
			assert(params.size() >= 1, "Member functions require at least one parameter.");
			addParam(l, params[0]);
			start = 1;
		}

		if (!resultBuiltIn)
			l.frame.createParameter(resultSize, false); // no dtor needed here.

		for (nat i = start; i < params.size(); i++)
			addParam(l, params[i]);

		l << prolog();

		if (param != Value())
			l << fnParam(param);
		l << fnCall(fn, Size::sPtr);

		l << epilog(); // preserves ptrA
		l << jmp(ptrA);

		return l;
	}
}
