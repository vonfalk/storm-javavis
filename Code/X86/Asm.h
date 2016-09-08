#pragma once
#include "../Register.h"
#include "../Output.h"
#include "../Operand.h"

namespace code {
	namespace x86 {

		// Marker that is easy to search for. Removed when we're done.
#define NOT_DONE assert(false, L"Not implemented yet!")

		// Find the register id of a register.
		nat registerId(Register r);

		// See if this value can be expressed in a single byte without loss. Assumes sign extension.
		bool singleByte(Word value);

		// Generates a SIB-byte that express the address: baseReg + scaledReg*scale.
		// NOTE: scale must be either 1, 2, 4 or 8.
		// NOTE: baseReg == ebp does _not_ work.
		// NOTE: scaledReg == esp does _not_ work.
		// When scaledReg == -1 (0xFF) no scaling is used. (scale must be 1)
		byte sibValue(byte baseReg, byte scaledReg, byte scale);
		inline byte sibValue(byte baseReg, byte scaledReg) { return sibValue(baseReg, scaledReg, 1); }
		inline byte sibValue(byte baseReg) { return sibValue(baseReg, -1); }

		// Assemble a modRmValue from its parts.
		byte modRmValue(byte mode, byte src, byte dest);

		// Compute 'modRm' for subOp, dest. Write to 'to'.
		void modRm(Output *to, byte subOp, const Operand &dest);

		// Describes a single instruction that takes either an immediate value, or a register.
		struct ImmRegInstr {
			// The op-code and mode when followed by an 8-bit immediate value. If 'modeImm8' == 0xFF
			// the imm8 is not used. If 'modeImm8' == 0x0F (prefix), the opImm8 is treated as a prefixed op-code.
			byte opImm8, modeImm8;
			// The op-code and mode when followed by a 32-bit immediate value.
			byte opImm32, modeImm32;
			// The op-code when using a register as source.
			byte opSrcReg;
			// The op-code when using a register as a destination.
			byte opDestReg;
		};

		// Describes a single instruction that takes either an immediate value, or a register. 8-bit operands.
		struct ImmRegInstr8 {
			// Op-code and mode when followed by an 8-bit immediate value.
			byte opImm, modeImm;
			// Op-code when using a register as source.
			byte opSrcReg;
			// Op-code when using a register as destination.
			byte opDestReg;
		};

		// Generates code for an instruction described by the ImmRegInstr. Do not support both src and
		// dest to be "strange" addressing modes. The following modes are supported:
		// register, *
		// *, register
		// *, constant
		// Use "immRegTransform" to expand op-codes to a move before the op-code, so that we can handle all combinations.
		void immRegInstr(Output *to, const ImmRegInstr &op, const Operand &dest, const Operand &src);
		void immRegInstr(Output *to, const ImmRegInstr8 &op, const Operand &dest, const Operand &src);
		void immRegInstr(Output *to, const ImmRegInstr8 &op8, const ImmRegInstr &op, const Operand &dest, const Operand &src);

	}
}
