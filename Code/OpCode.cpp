#include "stdafx.h"
#include "OpCode.h"

#include "Utils/HashMap.h"

using namespace code::op;

#define OP_CODE(x) { op::x, L ## #x }

namespace code {

	struct OpDesc {
		OpCode opCode;
		const wchar_t *name;
	};

	static OpDesc descs[] = {
		OP_CODE(mov),
		OP_CODE(lea),
		OP_CODE(push),
		OP_CODE(pop),
		OP_CODE(jmp),
		OP_CODE(call),
		OP_CODE(ret),
		OP_CODE(setCond),
		OP_CODE(fnParam),
		OP_CODE(fnCall),
		OP_CODE(add),
		OP_CODE(adc),
		OP_CODE(or),
		OP_CODE(and),
		OP_CODE(sub),
		OP_CODE(sbb),
		OP_CODE(xor),
		OP_CODE(cmp),
		OP_CODE(mul),
		OP_CODE(idiv),
		OP_CODE(udiv),
		OP_CODE(imod),
		OP_CODE(umod),
		OP_CODE(shl),
		OP_CODE(shr),
		OP_CODE(sar),
		OP_CODE(icast),
		OP_CODE(ucast),
		OP_CODE(fstp),
		OP_CODE(fistp),
		OP_CODE(fld),
		OP_CODE(fild),
		OP_CODE(faddp),
		OP_CODE(fsubp),
		OP_CODE(fmulp),
		OP_CODE(fdivp),
		OP_CODE(fwait),
		OP_CODE(retFloat),
		OP_CODE(dat),
		OP_CODE(addRef),
		OP_CODE(releaseRef),
		OP_CODE(prolog),
		OP_CODE(epilog),
		OP_CODE(beginBlock),
		OP_CODE(endBlock),
		OP_CODE(threadLocal),
	};

	const wchar_t *name(OpCode opCode) {
		static bool initialized = false;
		static const wchar_t *opNames[op::numOpCodes] = {};

		if (!initialized) {
			for (nat i = 0; i < ARRAY_SIZE(descs); i++) {
				opNames[descs[i].opCode] = descs[i].name;
			}
		}

		const wchar_t *found = opNames[opCode];
		assert(found);
		if (!found)
			found = L"INVALID";
		return found;
	}

}
