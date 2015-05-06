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

		void lea(Output &to, Params p, const Instruction &instr) {
			const Value &src = instr.src();
			const Value &dest = instr.dest();
			assert(dest.type() == Value::tRegister);
			nat regId = registerId(dest.reg());

			if (src.type() == Value::tReference) {
				// Special meaning, load the reference's index instead.
				to.putByte(0xB8 + regId);
				to.putRefId(src.reference());
			} else {
				to.putByte(0x8D);
				modRm(to, regId, src);
			}
		}

		void push(Output &to, Params p, const Instruction &instr) {
			// Size == 1 is covered by pushing whatever garbage is in the register. We need to keep the alignment
			// of the stack anyway!
			assert(instr.currentSize() <= 4, toS(instr) + L": Only size 4 is supported now.");
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
			case Value::tReference:
				to.putByte(0x68); // we need an entire 32-bits.
				to.putAddress(src.reference());
				break;
			case Value::tLabel:
				to.putByte(0x68);
				to.putAddress(src.label());
				break;
			default:
				assert(false);
				break;
			}
		}

		void pop(Output &to, Params p, const Instruction &instr) {
			assert(instr.currentSize() == 4); // more comes later!
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
				PLN(L"jmpCall not implemented for " << src);
				assert(false);
				break;
			}
		}

		static void jmpCall(bool call, Arena &arena, Output &output, const Value &src) {
			if (src.type() == Value::tRegister) {
				output.putByte(0xFF);
				modRm(output, call ? 2 : 4, src);
			} else {
				byte opCode = call ? 0xE8 : 0xE9;
				jmpCall(opCode, arena, output, src);
			}
		}

		static byte condOp(CondFlag c) {
			switch (c) {
			case ifAlways:
				assert(false);
				break;
			case ifOverflow:
				return 0x0;
			case ifNoOverflow:
				return 0x1;
			case ifEqual:
				return 0x4;
			case ifNotEqual:
				return 0x5;
			case ifBelow:
				return 0x2;
			case ifBelowEqual:
				return 0x6;
			case ifAboveEqual:
				return 0x3;
			case ifAbove:
				return 0x7;
			case ifLess:
				return 0xC;
			case ifLessEqual:
				return 0xE;
			case ifGreaterEqual:
				return 0xD;
			case ifGreater:
				return 0xF;
			}

			assert(false);
			PLN("Missing jmpcond: " << c);
			return 0;
		}

		void jmp(Output &to, Params p, const Instruction &instr) {
			CondFlag c = instr.src().condFlag();
			if (c == ifAlways) {
				jmpCall(false, p.arena, to, instr.dest());
			} else if (c == ifNever) {
				// nothing
			} else {
				byte op = 0x80 + condOp(c);
				to.putByte(0x0F);
				jmpCall(op, p.arena, to, instr.dest());
			}
		}

		void setCond(Output &to, Params p, const Instruction &instr) {
			CondFlag c = instr.src().condFlag();
			to.putByte(0x0F);
			to.putByte(0x90 + condOp(c));
			modRm(to, 0, instr.dest());
		}

		void call(Output &to, Params p, const Instruction &instr) {
			jmpCall(true, p.arena, to, instr.src());
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

		void mul(Output &to, Params p, const Instruction &instr) {
			assert(instr.dest().type() == Value::tRegister);
			to.putByte(0x0F);
			to.putByte(0xAF);
			modRm(to, registerId(instr.dest().reg()), instr.src());
		}


		/**
		 * Shift op-codes.
		 */

		static void shiftOp(Output &to, const Value &dest, const Value &src, byte subOp) {
			assert(dest.currentSize() < 8, "64-bit conversion failed.");

			byte c;
			bool is8 = dest.currentSize() == 1;

			switch (src.type()) {
			case Value::tConstant:
				c = byte(src.constant());
				if (c == 1) {
					to.putByte(is8 ? 0xD0 : 0xD1);
					modRm(to, subOp, dest);
				} else {
					to.putByte(is8 ? 0xC0 : 0xC1);
					modRm(to, subOp, dest);
					to.putByte(c);
				}
				break;
			case Value::tRegister:
				assert(src.reg() == cl, "Transform of shift-op failed.");

				to.putByte(is8 ? 0xD2 : 0xD3);
				modRm(to, subOp, dest);
				break;
			default:
				assert(false, "The transform was not run.");
				break;
			}
		}

		void shl(Output &to, Params p, const Instruction &instr) {
			shiftOp(to, instr.dest(), instr.src(), 4);
		}

		void shr(Output &to, Params p, const Instruction &instr) {
			shiftOp(to, instr.dest(), instr.src(), 5);
		}

		void sar(Output &to, Params p, const Instruction &instr) {
			shiftOp(to, instr.dest(), instr.src(), 7);
		}

		void dat(Output &to, Params p, const Instruction &instr) {
			const Value &v = instr.src();
			switch (v.type()) {
			case Value::tConstant:
				to.putSize(v.constant(), v.currentSize());
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

		void threadLocal(Output &to, Params p, const Instruction &instr) {
			to.putByte(0x64); // FS segment
		}
	}
}

#endif
