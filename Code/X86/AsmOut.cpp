#include "stdafx.h"
#include "AsmOut.h"
#include "Asm.h"
#include "OpTable.h"

namespace code {
	namespace x86 {

#define OUTPUT(x) { op::x, &x ## Out }

		typedef void (*OutputFn)(Output *to, Instr *instr);

		void movOut(Output *to, Instr *instr) {
			ImmRegInstr8 op8 = {
				0xC6, 0,
				0x88,
				0x8A,
			};
			ImmRegInstr op = {
				0x0, 0xFF, // Not supported
				0xC7, 0,
				0x89,
				0x8B,
			};
			immRegInstr(to, op8, op, instr->dest(), instr->src());
		}

		void addOut(Output *to, Instr *instr) {
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
			immRegInstr(to, op8, op, instr->dest(), instr->src());
		}

		void adcOut(Output *to, Instr *instr) {
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
			immRegInstr(to, op8, op, instr->dest(), instr->src());
		}

		void orOut(Output *to, Instr *instr) {
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
			immRegInstr(to, op8, op, instr->dest(), instr->src());
		}

		void andOut(Output *to, Instr *instr) {
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
			immRegInstr(to, op8, op, instr->dest(), instr->src());
		}

		void subOut(Output *to, Instr *instr) {
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
			immRegInstr(to, op8, op, instr->dest(), instr->src());
		}

		void sbbOut(Output *to, Instr *instr) {
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
			immRegInstr(to, op8, op, instr->dest(), instr->src());
		}

		void xorOut(Output *to, Instr *instr) {
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
			immRegInstr(to, op8, op, instr->dest(), instr->src());
		}

		void cmpOut(Output *to, Instr *instr) {
			ImmRegInstr8 op8 = {
				0x82, 7, // NOTE: it seems like this can also be encoded as 0x80 (preferred by some sources).
				0x38,
				0x3A
			};
			ImmRegInstr op = {
				0x83, 7,
				0x81, 7,
				0x39,
				0x3B
			};
			immRegInstr(to, op8, op, instr->dest(), instr->src());
		}

		void pushOut(Output *to, Instr *instr) {
			const Operand &src = instr->src();
			switch (src.type()) {
			case opConstant:
				if (singleByte(src.constant())) {
					to->putByte(0x6A);
					to->putByte(Byte(src.constant() & 0xFF));
				} else {
					to->putByte(0x68);
					to->putInt(Int(src.constant()));
				}
			case opRegister:
				to->putByte(0x50 + registerId(src.reg()));
				break;
			case opRelative:
				to->putByte(0xFF);
				modRm(to, 6, src);
				break;
			case opReference:
				NOT_DONE;
				// to->putByte(0x68);
				// to->putAddress(src.reference());
				break;
			case opLabel:
				to->putByte(0x68);
				to->putAddress(src.label());
				break;
			default:
				assert(false, L"Push does not support this operand type.");
				break;
			}
		}

		void popOut(Output *to, Instr *instr) {
			const Operand &dest = instr->dest();
			switch (dest.type()) {
			case opRegister:
				to->putByte(0x58 + registerId(dest.reg()));
				break;
			case opRelative:
				to->putByte(0x8F);
				modRm(to, 0, dest);
				break;
			default:
				assert(false, L"Pop does not support this operand type.");
			}
		}

		void pushFlagsOut(Output *to, Instr *instr) {
			to->putByte(0x9C);
		}

		void popFlagsOut(Output *to, Instr *instr) {
			to->putByte(0x9D);
		}

		void retOut(Output *to, Instr *instr) {
			to->putByte(0xC3);
		}

		void setCondOut(Output *to, Instr *instr) {
			CondFlag c = instr->src().condFlag();
			to->putByte(0x0F);
			to->putByte(0x90 + condOp(c));
			modRm(to, 0, instr->dest());
		}

		static void jmpCall(byte opCode, Output *to, const Operand &src) {
			switch (src.type()) {
			case opConstant:
				to->putByte(opCode);
				to->putRelativeStatic(src.constant());
				break;
			case opLabel:
				to->putByte(opCode);
				to->putRelative(src.label());
				break;
			case opReference:
				NOT_DONE;
				// to->putByte(opCode);
				// to->putRelative(...);
				break;
			default:
				assert(false, L"JmpCall not implemented for " + ::toS(src));
				break;
			}
		}

		static void jmpCall(bool call, Output *to, const Operand &src) {
			if (src.type() == opRegister) {
				to->putByte(0xFF);
				modRm(to, call ? 2 : 4, src);
			} else {
				byte opCode = call ? 0xE8 : 0xE9;
				jmpCall(opCode, to, src);
			}
		}

		void jmpOut(Output *to, Instr *instr) {
			CondFlag c = instr->src().condFlag();
			if (c == ifAlways) {
				jmpCall(false, to, instr->dest());
			} else if (c == ifNever) {
				// Nothing.
			} else {
				byte op = 0x80 + condOp(c);
				to->putByte(0x0F);
				jmpCall(op, to, instr->dest());
			}
		}

		void callOut(Output *to, Instr *instr) {
			jmpCall(true, to, instr->src());
		}

		static void shiftOp(Output *to, const Operand &dest, const Operand &src, byte subOp) {
			byte c;
			bool is8 = dest.size() == Size::sByte;

			switch (src.type()) {
			case opConstant:
				c = byte(src.constant());
				if (c == 1) {
					to->putByte(is8 ? 0xD0 : 0xD1);
					modRm(to, subOp, dest);
				} else {
					to->putByte(is8 ? 0xC0 : 0xC1);
					modRm(to, subOp, dest);
					to->putByte(c);
				}
				break;
			case opRegister:
				assert(src.reg() == cl, L"Transform of shift-op failed.");
				to->putByte(is8 ? 0xD2 : 0xD3);
				modRm(to, subOp, dest);
				break;
			default:
				assert(false, L"The transformation was not run.");
				break;
			}
		}

		void shlOut(Output *to, Instr *instr) {
			shiftOp(to, instr->dest(), instr->src(), 4);
		}

		void shrOut(Output *to, Instr *instr) {
			shiftOp(to, instr->dest(), instr->src(), 5);
		}

		void sarOut(Output *to, Instr *instr) {
			shiftOp(to, instr->dest(), instr->src(), 7);
		}

		void icastOut(Output *to, Instr *instr) {
			nat sFrom = instr->src().size().size32();
			nat sTo = instr->dest().size().size32();
			Engine &e = instr->engine();

			assert(same(instr->dest().reg(), ptrA), L"Only rax, eax, or al supported as a target for icast.");
			bool srcEax = instr->src().type() == opRegister && same(instr->src().reg(), ptrA);

			if (sFrom == 1 && sTo == 4) {
				// movsx
				to->putByte(0x0F);
				to->putByte(0xBE);
				modRm(to, registerId(eax), instr->src());
			} else if (sFrom == 4 && sTo == 8) {
				// mov (if needed).
				if (!srcEax)
					movOut(to, mov(e, eax, instr->src()));
				// cdq
				to->putByte(0x99);
			} else if (sFrom == 8 && sTo == 4) {
				if (!srcEax)
					movOut(to, mov(e, eax, low32(instr->src())));
			} else if (sFrom ==4 && sTo == 1) {
				if (!srcEax)
					movOut(to, mov(e, eax, instr->src()));
			} else if (sFrom == 8 && sTo == 8) {
				if (!srcEax) {
					movOut(to, mov(e, eax, low32(instr->src())));
					movOut(to, mov(e, edx, high32(instr->src())));
				}
			} else if (sFrom == sTo) {
				if (!srcEax)
					movOut(to, mov(e, eax, instr->src()));
			} else {
				assert(false, L"Unsupported icast mode: " + ::toS(instr));
			}
		}

		void ucastOut(Output *to, Instr *instr) {
			nat sFrom = instr->src().size().size32();
			nat sTo = instr->dest().size().size32();
			Engine &e = instr->engine();

			assert(same(instr->dest().reg(), ptrA), L"Only rax, eax, or al supported as a target for icast.");
			bool srcEax = instr->src().type() == opRegister && same(instr->src().reg(), ptrA);

			if (sFrom == 1 && sTo == 4) {
				// movzx
				to->putByte(0x0F);
				to->putByte(0xB6);
				modRm(to, registerId(eax), instr->src());
			} else if (sFrom == 4 && sTo == 8) {
				// mov (if needed).
				if (!srcEax)
					movOut(to, mov(e, eax, instr->src()));
				// xor edx, edx
				to->putByte(0x33);
				to->putByte(0xD2);
			} else if (sFrom == 8 && sTo == 4) {
				if (!srcEax)
					movOut(to, mov(e, eax, low32(instr->src())));
			} else if (sFrom ==4 && sTo == 1) {
				if (!srcEax)
					movOut(to, mov(e, eax, instr->src()));
			} else if (sFrom == 8 && sTo == 8) {
				if (!srcEax) {
					movOut(to, mov(e, eax, low32(instr->src())));
					movOut(to, mov(e, edx, high32(instr->src())));
				}
			} else if (sFrom == sTo) {
				if (!srcEax)
					movOut(to, mov(e, eax, instr->src()));
			} else {
				assert(false, L"Unsupported icast mode: " + ::toS(instr));
			}
		}

		const OpEntry<OutputFn> outputMap[] = {
			OUTPUT(mov),
			OUTPUT(add),
			OUTPUT(adc),
			OUTPUT(or),
			OUTPUT(and),
			OUTPUT(sub),
			OUTPUT(sbb),
			OUTPUT(xor),
			OUTPUT(cmp),
			OUTPUT(push),
			OUTPUT(pop),
			OUTPUT(pushFlags),
			OUTPUT(popFlags),
			OUTPUT(setCond),
			OUTPUT(jmp),
			OUTPUT(call),
			OUTPUT(ret),
			OUTPUT(shl),
			OUTPUT(shr),
			OUTPUT(sar),
			OUTPUT(icast),
			OUTPUT(ucast),
		};

		void output(Listing *src, Output *to) {
			static OpTable<OutputFn> t(outputMap, ARRAY_COUNT(outputMap));

			for (Nat i = 0; i < src->count(); i++) {
				Array<Label> *labels = src->labels(i);
				for (nat l = 0; labels != null && l < labels->count(); l++) {
					to->mark(labels->at(l));
				}

				Instr *instr = src->at(i);
				OutputFn fn = t[instr->op()];
				if (fn) {
					(*fn)(to, instr);
				} else {
					assert(false, L"Unsupported op-code: " + String(name(instr->op())));
				}
			}
		}

	}
}
