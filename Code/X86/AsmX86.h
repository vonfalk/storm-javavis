#pragma once

#ifdef X86
#include "X86/MachineCodeX86.h"

namespace code {
	namespace machineX86 {
		void mov(Output &to, Params p, const Instruction &instr);
		void lea(Output &to, Params p, const Instruction &instr);
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

		void shl(Output &to, Params p, const Instruction &instr);
		void shr(Output &to, Params p, const Instruction &instr);
		void sar(Output &to, Params p, const Instruction &instr);

		void dat(Output &to, Params p, const Instruction &instr);

		void threadLocal(Output &to, Params p, const Instruction &instr);
	}
}

#endif
