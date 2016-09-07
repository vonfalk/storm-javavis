#pragma once

#ifdef X86
#include "X86/MachineCodeX86.h"

namespace code {
	namespace machineX86 {
		void mov(Output &to, Params p, const Instruction &instr);
		void lea(Output &to, Params p, const Instruction &instr);

		// Note: push is one of the few op-codes that actually manages 64-bit operands at this level.
		void push(Output &to, Params p, const Instruction &instr);
		void pop(Output &to, Params p, const Instruction &instr);

		void jmp(Output &to, Params p, const Instruction &instr);
		void call(Output &to, Params p, const Instruction &instr);
		void ret(Output &to, Params p, const Instruction &instr);
		void setCond(Output &to, Params p, const Instruction &instr);

		void add(Output &to, Params p, const Instruction &instr);
		void adc(Output &to, Params p, const Instruction &instr);
		void or(Output &to, Params p, const Instruction &instr);
		void and(Output &to, Params p, const Instruction &instr);
		void sub(Output &to, Params p, const Instruction &instr);
		void sbb(Output &to, Params p, const Instruction &instr);
		void xor(Output &to, Params p, const Instruction &instr);
		void cmp(Output &to, Params p, const Instruction &instr);
		void mul(Output &to, Params p, const Instruction &instr);
		void idiv(Output &to, Params p, const Instruction &instr);
		void udiv(Output &to, Params p, const Instruction &instr);

		void shl(Output &to, Params p, const Instruction &instr);
		void shr(Output &to, Params p, const Instruction &instr);
		void sar(Output &to, Params p, const Instruction &instr);

		void icast(Output &to, Params p, const Instruction &instr);
		void ucast(Output &to, Params p, const Instruction &instr);

		void fstp(Output &to, Params p, const Instruction &instr);
		void fistp(Output &to, Params p, const Instruction &instr);
		void fld(Output &to, Params p, const Instruction &instr);
		void fild(Output &to, Params p, const Instruction &instr);
		void faddp(Output &to, Params p, const Instruction &instr);
		void fsubp(Output &to, Params p, const Instruction &instr);
		void fmulp(Output &to, Params p, const Instruction &instr);
		void fdivp(Output &to, Params p, const Instruction &instr);
		void fcompp(Output &to, Params p, const Instruction &instr);
		void fwait(Output &to, Params p, const Instruction &instr);

		void dat(Output &to, Params p, const Instruction &instr);

		void threadLocal(Output &to, Params p, const Instruction &instr);
	}
}

#endif