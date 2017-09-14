#pragma once
#include "../Reg.h"
#include "../Output.h"
#include "../Operand.h"

namespace code {
	namespace x64 {
		STORM_PKG(core.asm.x64);

		/**
		 * X86-64 specific registers.
		 *
		 * TODO: Expose to Storm somehow.
		 */
		extern const Reg ptrD;
		extern const Reg ptrSi;
		extern const Reg ptrDi;
		extern const Reg ptr8;
		extern const Reg ptr9;
		extern const Reg ptr10;
		extern const Reg ptr11;
		extern const Reg ptr12;
		extern const Reg ptr13;
		extern const Reg ptr14;
		extern const Reg ptr15;
		extern const Reg dl;
		extern const Reg sil;
		extern const Reg dil;
		extern const Reg edx;
		extern const Reg esi;
		extern const Reg edi;
		extern const Reg rdx;
		extern const Reg rsi;
		extern const Reg rdi;
		extern const Reg e8;
		extern const Reg e9;
		extern const Reg e10;
		extern const Reg e11;
		extern const Reg e12;
		extern const Reg e13;
		extern const Reg e14;
		extern const Reg e15;
		extern const Reg r8;
		extern const Reg r9;
		extern const Reg r10;
		extern const Reg r11;
		extern const Reg r12;
		extern const Reg r13;
		extern const Reg r14;
		extern const Reg r15;

		// Convert to names.
		const wchar *nameX64(Reg r);

		// Register ID.
		nat registerId(Reg r);

		// Does 'value' fit in a single byte?
		bool singleByte(Word value);

		// Does 'value' fit in a 32-bit word?
		bool singleInt(Word value);

		// Find a unused register given a set of used registers.
		Reg unusedReg(RegSet *in);

		// Description of an op-code.
		struct OpCode {
			// The actual op-code. Maximum 2 bytes. If only one byte is required, the first byte is
			// 0. (the byte 0x00 is the ADD instruction which is 1 byte long. Thus 0x00 0x00
			// uniquely identifies ADD).
			byte op1;
			byte op2;
		};

		// Create OpCode objects.
		inline OpCode opCode(byte op) {
			OpCode r = { 0x00, op };
			return r;
		}

		inline OpCode opCode(byte op1, byte op2) {
			OpCode r = { op1, op2 };
			return r;
		}

		// Output an opcode.
		void put(Output *to, OpCode op);

		// Output an instruction with a ModRm modifier afterwards. Emits a REX prefix if necessary.
		void modRm(Output *to, OpCode op, const Operand &dest, const Operand &src);
		void modRm(Output *to, OpCode op, nat mode, const Operand &dest);

		// Describes a single instruction that takes an immediate value or a register, along with a ModRm operand.
		struct ImmRegInstr {
			// OP-code and mode when followed by an 8-bit immediate value. If 'modeImm' == 0xFF, imm8 is not used.
			OpCode opImm8;
			byte modeImm8;

			// OP-code and mode when followed by a 32-bit immediate value.
			OpCode opImm32;
			byte modeImm32;

			// OP-code when using a register as a source.
			OpCode opSrcReg;

			// OP-code when using a register as a destination.
			OpCode opDestReg;
		};

		// Describes a single instruction with 8 bit operands.
		struct ImmRegInstr8 {
			OpCode opImm;
			byte modeImm;

			OpCode opSrcReg;
			OpCode opDestReg;
		};

		// Emit an instruction represented by ImmRegInstr. Supports the following addressing modes:
		// register, *
		// *, register
		// *, immediate
		void immRegInstr(Output *to, const ImmRegInstr &op, const Operand &dest, const Operand &src);
		void immRegInstr(Output *to, const ImmRegInstr8 &op, const Operand &dest, const Operand &src);
		void immRegInstr(Output *to, const ImmRegInstr8 &op8, const ImmRegInstr &op, const Operand &dest, const Operand &src);

	}
}
