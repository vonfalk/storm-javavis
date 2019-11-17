#pragma once
#include "Utils/Bitmask.h"

namespace code {

	/**
	 * Declare all virtual op-codes.
	 */
	namespace op {
		STORM_PKG(core.asm);

		enum OpCode {
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
			fnParamRef,
			fnCall,
			fnCallRef,
			fnRet,
			fnRetRef,
			STORM_NAME(bor, or),
			STORM_NAME(band, and),
			STORM_NAME(bxor, xor),
			STORM_NAME(bnot, not),
			add,
			adc,
			sub,
			sbb,
			cmp,
			test,
			mul,
			idiv,
			udiv,
			imod,
			umod,
			shl,
			shr,
			sar,
			swap,
			icast,
			ucast,

			// Floating point.
			fstp,
			fistp,
			fld,
			fild,
			fldz,
			faddp,
			fsubp,
			fmulp,
			fdivp,
			fcompp,
			fwait,

			// Data
			dat,
			lblOffset,
			align,

			// Function prolog/epilog.
			prolog,
			epilog,

			// Note that a register has been preserved somewhere. Used to generate debugging information.
			preserve,

			// Source code reference, used for debug information or other transformations.
			location,

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

	const wchar *name(op::OpCode op);
	DestMode destMode(op::OpCode op);
}
