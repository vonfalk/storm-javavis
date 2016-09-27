#pragma once
#include "Utils/Bitmask.h"

namespace code {

	/**
	 * Declare all virtual op-codes.
	 */
	namespace op {
		STORM_PKG(core.asm.op);

		enum Code {
			nop,
			mov,
			lea,
			push,
			pop,
			pushFlags,
			popFlags,
			jmp,
			call,
			ret,
			setCond,
			fnParam,
			fnCall,
			add,
			adc,
			or,
			and,
			sub,
			sbb,
			xor,
			cmp,
			mul,
			idiv,
			udiv,
			imod,
			umod,
			shl,
			shr,
			sar,
			icast,
			ucast,

			// Floating point.
			fstp,
			fistp,
			fld,
			fild,
			faddp,
			fsubp,
			fmulp,
			fdivp,
			fcompp,
			fwait,
			callFloat,
			retFloat,
			fnCallFloat,

			// Data
			dat,

			// Function prolog/epilog.
			prolog,
			epilog,

			// Blocks.
			beginBlock,
			endBlock,

			// Make the next memory operation work on the thread local storage.
			threadLocal,

			// Keep last
			numOpCodes,
		};
	}

	/**
	 * What is done to the 'dest' of the instructions?
	 */
	enum DestMode {
		destNone = 0x0,
		destRead = 0x1,
		destWrite = 0x2,
	};

	BITMASK_OPERATORS(DestMode);

	const wchar *name(op::Code op);
	DestMode destMode(op::Code op);
}
