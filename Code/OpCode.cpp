#include "stdafx.h"
#include "OpCode.h"
#include "OpTable.h"

using namespace code::op;

namespace code {

	struct OpInfo {
		const wchar *name;
		DestMode mode;
	};

#define OP_CODE(x, mode) { op::x, { L ## #x, mode } }

	static OpEntry<OpInfo> descs[] = {
		OP_CODE(nop, destNone),
		OP_CODE(mov, destWrite),
		OP_CODE(lea, destWrite),
		OP_CODE(push, destNone),
		OP_CODE(pop, destWrite),
		OP_CODE(jmp, destRead),
		OP_CODE(call, destRead),
		OP_CODE(ret, destNone),
		OP_CODE(setCond, destWrite),
		OP_CODE(fnParam, destNone),
		OP_CODE(fnParamRef, destNone),
		OP_CODE(fnCall, destRead),
		OP_CODE(add, destWrite),
		OP_CODE(adc, destWrite),
		OP_CODE(or, destWrite),
		OP_CODE(and, destWrite),
		OP_CODE(sub, destWrite),
		OP_CODE(sbb, destWrite),
		OP_CODE(xor, destWrite),
		OP_CODE(cmp, destWrite),
		OP_CODE(mul, destWrite),
		OP_CODE(idiv, destWrite),
		OP_CODE(udiv, destWrite),
		OP_CODE(imod, destWrite),
		OP_CODE(umod, destWrite),
		OP_CODE(shl, destWrite),
		OP_CODE(shr, destWrite),
		OP_CODE(sar, destWrite),
		OP_CODE(icast, destWrite),
		OP_CODE(ucast, destWrite),
		OP_CODE(fstp, destWrite),
		OP_CODE(fistp, destWrite),
		OP_CODE(fld, destNone),
		OP_CODE(fild, destNone),
		OP_CODE(faddp, destNone),
		OP_CODE(fsubp, destNone),
		OP_CODE(fmulp, destNone),
		OP_CODE(fdivp, destNone),
		OP_CODE(fcompp, destNone),
		OP_CODE(fwait, destNone),
		OP_CODE(callFloat, destRead),
		OP_CODE(retFloat, destNone),
		OP_CODE(fnCallFloat, destRead),
		OP_CODE(dat, destNone),
		OP_CODE(prolog, destNone),
		OP_CODE(epilog, destNone),
		OP_CODE(beginBlock, destNone),
		OP_CODE(endBlock, destNone),
		OP_CODE(threadLocal, destNone),
	};

	static const OpInfo &info(op::Code op) {
		static OpTable<OpInfo> ops(descs, ARRAY_COUNT(descs));
		return ops[op];
	}

	const wchar *name(op::Code op) {
		return info(op).name;
	}

	DestMode destMode(op::Code op) {
		return info(op).mode;
	}

}
