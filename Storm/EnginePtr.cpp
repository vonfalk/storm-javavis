#include "stdafx.h"
#include "EnginePtr.h"
#include "Value.h"
#include "Engine.h"

namespace storm {

#ifdef X86

	code::Listing enginePtrThunk(Engine &e, const Value &returnType, code::Ref fn) {
		using namespace code;

		Listing l;

		if (returnType.returnInReg()) {
			// The old pointer and the 0 constant will nicely fit into the 'returnData' member.
			l << push(natPtrConst(0));
			l << push(e.engineRef);
		} else {
			// The first parameter is, and has to be, a pointer to the returned object.
			l << mov(ptrA, ptrRel(ptrStack, Offset::sPtr)); // read the return value ptr
			l << push(e.engineRef);
			l << push(ptrA); // store the return value ptr once more.
		}

		l << call(fn, returnType.size());
		l << add(ptrStack, natPtrConst(Size::sPtr * 2));
		l << ret(returnType.size());

		return l;
	}

#endif
}
