#include "stdafx.h"

#ifdef X86
#include "MachineCodeX86.h"
#include "Instruction.h"
#include "Output.h"

namespace code {
	namespace machineX86 {


		void mov(Output &to, Params p, const Instruction &instr) {
			ImmRegInstr8 op8 = {
				0xC6, 0,
				0x88,
				0x8A
			};
			ImmRegInstr op = {
				0x0, 0xFF, // No support for imm8
				0xC7, 0,
				0x89,
				0x8B
			};
			immRegInstr(to, op8, op, instr.dest(), instr.src());
		}

		void push(Output &to, Params p, const Instruction &instr) {
			assert(instr.size() == 4); // more comes later!
			const Value &src = instr.src();
			switch (src.type()) {
				case Value::tConstant:
					if (singleByte(src.constant())) {
						to.putByte(0x6A);
						to.putByte(Byte(src.constant() & 0xFF));
					} else {
						to.putByte(0x68);
						to.putInt(cpuNat(src.constant()));
					}
					break;
				case Value::tRegister:
					to.putByte(0x50 + registerId(src.reg()));
					break;
				case Value::tRelative:
					to.putByte(0xFF);
					modRm(to, 6, src);
					break;
				default:
					assert(false);
					break;
			}
		}

		void pop(Output &to, Params p, const Instruction &instr) {
			assert(instr.size() == 4); // more comes later!
			const Value &dest = instr.dest();
			switch (dest.type()) {
				case Value::tRegister:
					to.putByte(0x58 + registerId(dest.reg()));
					break;
				case Value::tRelative:
					to.putByte(0x8F);
					modRm(to, 0, dest);
					break;
				default:
					assert(false);
					break;
			}
		}

		static void jmpCall(byte opCode, Arena &arena, Output &output, const Value &src) {
			switch (src.type()) {
				case Value::tConstant:
					output.putByte(opCode);
					output.putRelative(cpuNat(src.constant()));
					break;
				case Value::tLabel:
					output.putByte(opCode);
					output.putRelative(src.label());
					break;
				case Value::tReference:
					output.putByte(opCode);
					output.putRelative(src.reference());
					break;
				default:
					assert(false);
					break;
			}
		}

		void jmp(Output &to, Params p, const Instruction &instr) {
			jmpCall(0xE9, p.arena, to, instr.src());
		}

		void call(Output &to, Params p, const Instruction &instr) {
			jmpCall(0xE8, p.arena, to, instr.src());
		}

		void ret(Output &to, Params p, const Instruction &instr) {
			to.putByte(0xC3);
		}

		void add(Output &to, Params p, const Instruction &instr) {
			ImmRegInstr8 op8 = {
				0x82, 0,
				0x00,
				0x02,
			};
			ImmRegInstr op = {
				0x83, 0,
				0x81, 0,
				0x01,
				0x03
			};
			immRegInstr(to, op8, op, instr.dest(), instr.src());
		}

		void adc(Output &to, Params p, const Instruction &instr) {
			ImmRegInstr8 op8 = {
				0x82, 2,
				0x10,
				0x12
			};
			ImmRegInstr op = {
				0x83, 2,
				0x81, 2,
				0x11,
				0x13
			};
			immRegInstr(to, op8, op, instr.dest(), instr.src());
		}

		void or(Output &to, Params p, const Instruction &instr) {
			ImmRegInstr8 op8 = {
				0x82, 1,
				0x08,
				0x0A
			};
			ImmRegInstr op = {
				0x83, 1,
				0x81, 1,
				0x09,
				0x0B
			};
			immRegInstr(to, op8, op, instr.dest(), instr.src());
		}

		void and(Output &to, Params p, const Instruction &instr) {
			ImmRegInstr8 op8 = {
				0x82, 4,
				0x20,
				0x22
			};
			ImmRegInstr op = {
				0x83, 4,
				0x81, 4,
				0x21,
				0x23
			};
			immRegInstr(to, op8, op, instr.dest(), instr.src());
		}

		void sub(Output &to, Params p, const Instruction &instr) {
			ImmRegInstr8 op8 = {
				0x82, 5,
				0x28,
				0x2A
			};
			ImmRegInstr op = {
				0x83, 5,
				0x81, 5,
				0x29,
				0x2B
			};
			immRegInstr(to, op8, op, instr.dest(), instr.src());
		}

		void sbb(Output &to, Params p, const Instruction &instr) {
			ImmRegInstr8 op8 = {
				0x82, 2,
				0x18,
				0x1A
			};
			ImmRegInstr op = {
				0x83, 2,
				0x81, 2,
				0x19,
				0x1B
			};
			immRegInstr(to, op8, op, instr.dest(), instr.src());
		}

		void xor(Output &to, Params p, const Instruction &instr) {
			ImmRegInstr8 op8 = {
				0x82, 6,
				0x30,
				0x32
			};
			ImmRegInstr op = {
				0x83, 6,
				0x81, 6,
				0x31,
				0x33
			};
			immRegInstr(to, op8, op, instr.dest(), instr.src());
		}

		void cmp(Output &to, Params p, const Instruction &instr) {
			ImmRegInstr8 op8 = {
				0x82, 7,
				0x38,
				0x3A
			};
			ImmRegInstr op = {
				0x83, 7,
				0x81, 7,
				0x39,
				0x3B
			};
			immRegInstr(to, op8, op, instr.dest(), instr.src());
		}

		void dat(Output &to, Params p, const Instruction &instr) {
			const Value &v = instr.src();
			switch (v.type()) {
				case Value::tConstant:
					to.putSize(v.constant(), v.sizeType());
					return;
				case Value::tLabel:
					to.putAddress(v.label());
					return;
				case Value::tReference:
					to.putAddress(v.reference());
					return;
			}
			assert(false);
		}

		void addRef(Output &to, Params p, const Instruction &instr) {
			assert(false); // Does not correctly call the function, no parameters are used!
			//jmpCall(0xE9, p.arena, to, Value((cpuNat)p.arena.addRef));
		}

		void releaseRef(Output &to, Params p, const Instruction &instr) {
			assert(false); // Does not correctly call the function, no parameters are used!
			//jmpCall(0xE9, p.arena, to, Value((cpuNat)p.arena.releaseRef));
		}

		void threadLocal(Output &to, Params p, const Instruction &instr) {
			to.putByte(0x64); // FS segment
		}
	}
}

#endif