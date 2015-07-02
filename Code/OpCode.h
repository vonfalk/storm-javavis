#pragma once

namespace code {

	// Declares all virtual OP-codes.
	namespace op {

		enum OpCode {
			mov,
			lea,
			push,
			pop,
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

			// High-level op-codes
			addRef,
			releaseRef,

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
	typedef op::OpCode OpCode;
	const wchar_t *name(OpCode opCode);
}
