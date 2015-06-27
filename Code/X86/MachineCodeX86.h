#pragma once

#ifdef X86
#include "Instruction.h"
#include "Listing.h"
#include "Register.h"
#include "Block.h"
#include "Seh.h"
#include "VariableX86.h"
#include "TransformX86.h"
#include "UsedRegisters.h"

namespace code {

	class Binary;

	namespace machineX86 {

		// Note on registers in this implementation:
		// For 64-bit integers, the registers are paired as follows: (low, high)
		// eax, edx (this is actually how MSVC2008 returns int64:s!)
		// ebx, esi
		// ecx, edi
		//
		// Registers eax, ecx, and edx are caller saved while the rest are callee saved.

		// New registers for the X86 backend!
		extern const Register ptrD, ptrSi, ptrDi;
		extern const Register dl;
		extern const Register edx, esi, edi;

		// Get the Transform fn for 'op'
		TransformFn transformFn(OpCode op);

		// Registers that function calls does not preserve in cdecl on x86.
		vector<Register> regsNotPreserved();

		// All base registers (ie not the ones used to extend base registers).
		vector<Register> regsBase();

		// Add all 64-bit pairs to the Registers object.
		void add64(Registers &r);

		// Does the given listing contain any 64-bit instructions?
		bool has64(const Listing &l);

		// Get the register containing the high bits of a 64-bit virtual register.
		Register high32(Register r);
		Register low32(Register r);
		Value high32(const Value &v);
		Value low32(const Value &v);

		const wchar_t* name(Register r);

		nat registerId(Register r);

		// Calculate the modRm value for the desired mode, src and dest. src is the parameter sometimes used for
		// different instructions sharing op-code.
		byte modRmValue(byte mode, byte src, byte dest);

		// Generate the entire modRm byte(s).
		void modRm(Output &to, byte subOp, const Value &dest);

		// See if the value can be expressed as a single byte, assuming 32-bit sign-extension.
		bool singleByte(Word value);

		// Parameters to all asm generating functions.
		struct Params {
			Arena &arena;
			const Frame &frame;
			// State &state;
		};


		// Describes a single instruction that takes either an immediate value, or a register.
		struct ImmRegInstr {
			// The op-code and mode when followed by an 8-bit immediate value. If modeImm8 == 0xFF the imm8 is not used.
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
		void immRegInstr(Output &to, const ImmRegInstr &op, const Value &dest, const Value &src);
		void immRegInstr(Output &to, const ImmRegInstr8 &op, const Value &dest, const Value &src);
		void immRegInstr(Output &to, const ImmRegInstr8 &op8, const ImmRegInstr &op, const Value &dest, const Value &src);

	}

}

#endif
