#include "stdafx.h"
#include "OpCode.h"

#include "OpTable.h"
#include "Utils/HashMap.h"

using namespace code::op;

#define OP_CODE(x) { op::x, L ## #x }

namespace code {

	static OpEntry<const wchar_t *> descs[] = {
		OP_CODE(mov),
		OP_CODE(lea),
		OP_CODE(push),
		OP_CODE(pop),
		OP_CODE(jmp),
		OP_CODE(call),
		OP_CODE(ret),
		OP_CODE(setCond),
		OP_CODE(fnParam),
		OP_CODE(fnParamRef),
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
		OP_CODE(fcompp),
		OP_CODE(fwait),
		OP_CODE(callFloat),
		OP_CODE(retFloat),
		OP_CODE(fnCallFloat),
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
		static OpTable<const wchar_t *> names(descs, ARRAY_COUNT(descs));

		const wchar_t *found = names[opCode];
		if (!found)
			found = L"INVALID";
		return found;
	}

}