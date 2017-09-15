#include "stdafx.h"
#include "AsmOut.h"
#include "Asm.h"
#include "../OpTable.h"

namespace code {
	namespace x64 {

		void pushOut(Output *to, Instr *instr) {
			const Operand &src = instr->src();
			switch (src.type()) {
			case opConstant:
				assert(singleInt(src.constant()), L"Should be solved by RemoveInvalid.");
				if (singleByte(src.constant())) {
					to->putByte(0x6A);
					to->putByte(Byte(src.constant() & 0xFF));
				} else {
					to->putByte(0x68);
					to->putInt(Int(src.constant()));
				}
				break;
			case opRegister:
			{
				nat reg = registerId(src.reg());
				if (reg >= 8) {
					to->putByte(0x41); // REX.B
					reg -= 8;
				}
				to->putByte(0x50 + reg);
				break;
			}
			case opRelative:
			case opReference:
			case opObjReference:
				modRm(to, opCode(0xFF), false, 6, src);
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
			{
				nat reg = registerId(dest.reg());
				if (reg >= 8) {
					to->putByte(0x41); // REX.B
					reg -= 8;
				}
				to->putByte(0x58 + reg);
				break;
			}
			case opRelative:
				modRm(to, opCode(0x8F), false, 0, dest);
				break;
			default:
				assert(false, L"Pop does not support this operand type.");
				break;
			}
		}

		void movOut(Output *to, Instr *instr) {
			ImmRegInstr8 op8 = {
				opCode(0xC6), 0,
				opCode(0x88),
				opCode(0x8A),
			};
			ImmRegInstr op = {
				opCode(0x00), 0xFF, // Not supported
				opCode(0xC7), 0,
				opCode(0x89),
				opCode(0x8B),
			};
			immRegInstr(to, op8, op, instr->dest(), instr->src());
		}

		void addOut(Output *to, Instr *instr) {
			ImmRegInstr8 op8 = {
				opCode(0x82), 0,
				opCode(0x00),
				opCode(0x02),
			};
			ImmRegInstr op = {
				opCode(0x83), 0,
				opCode(0x81), 0,
				opCode(0x01),
				opCode(0x03)
			};
			immRegInstr(to, op8, op, instr->dest(), instr->src());
		}

		void adcOut(Output *to, Instr *instr) {
			ImmRegInstr8 op8 = {
				opCode(0x82), 2,
				opCode(0x10),
				opCode(0x12)
			};
			ImmRegInstr op = {
				opCode(0x83), 2,
				opCode(0x81), 2,
				opCode(0x11),
				opCode(0x13)
			};
			immRegInstr(to, op8, op, instr->dest(), instr->src());
		}

		void borOut(Output *to, Instr *instr) {
			ImmRegInstr8 op8 = {
				opCode(0x82), 1,
				opCode(0x08),
				opCode(0x0A)
			};
			ImmRegInstr op = {
				opCode(0x83), 1,
				opCode(0x81), 1,
				opCode(0x09),
				opCode(0x0B)
			};
			immRegInstr(to, op8, op, instr->dest(), instr->src());
		}

		void bandOut(Output *to, Instr *instr) {
			ImmRegInstr8 op8 = {
				opCode(0x82), 4,
				opCode(0x20),
				opCode(0x22)
			};
			ImmRegInstr op = {
				opCode(0x83), 4,
				opCode(0x81), 4,
				opCode(0x21),
				opCode(0x23)
			};
			immRegInstr(to, op8, op, instr->dest(), instr->src());
		}

		void bnotOut(Output *to, Instr *instr) {
			const Operand &dest = instr->dest();
			if (dest.size() == Size::sByte) {
				modRm(to, opCode(0xF6), false, 2, dest);
			} else {
				modRm(to, opCode(0xF7), wide(dest), 2, dest);
			}
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

		void bxorOut(Output *to, Instr *instr) {
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

		void leaOut(Output *to, Instr *instr) {
			Operand src = instr->src();
			Operand dest = instr->dest();
			assert(dest.type() == opRegister);
			nat regId = registerId(dest.reg());

			if (src.type() == opReference) {
				// Special meaning, load the RefSource instead.
				// Issues a 'mov' operation to simply load the address of the refsource.
				modRm(to, opCode(0x8B), true, regId, objPtr(src.refSource()));
			} else {
				modRm(to, opCode(0x8D), true, regId, src);
			}
		}

		static void jmpCall(Output *to, bool call, const Operand &src) {
			switch (src.type()) {
			case opConstant:
				assert(false, L"Calling or jumping to constant values are not supported. Use labels or references!");
				break;
			case opLabel:
				to->putByte(call ? 0xE8 : 0xE9);
				to->putRelative(src.label());
				break;
			case opReference:
				// We have two options of encoding here, depending on if the reference is currently
				// within the 2GB we can address using the 32 bit operand. We want to use the short
				// encoding if possible (as that is probably faster, and will occur quite frequently
				// in practice). This is handled by the implementation in 'Refs.cpp'. This
				// implementation dictates that the short variant shall be encoded with an unneeded
				// REX prefix (we set the REX.W bit, so that we pretend that we mean something).
				//
				// We start by outputting the short version of the instruction so that the 'Refs'
				// implementation is able to deduce if we're encoding a jump or a call
				// instruction. Then we let that implementation deal with the fact that we might
				// actually need the long version of the jump/call.
				to->putByte(0x48); // REX.W
				to->putByte(call ? 0xE8 : 0xE9);
				to->putGc(GcCodeRef::jump, 4, src.ref());
				break;
			case opRegister:
			case opRelative:
				modRm(to, opCode(0x0F), false, call ? 2 : 4, src);
				break;
			default:
				assert(false, L"Unsupported operand used for 'call' or 'jump'.");
				break;
			}
		}

		void jmpOut(Output *to, Instr *instr) {
			CondFlag c = instr->src().condFlag();
			if (c == ifAlways) {
				jmpCall(to, false, instr->dest());
			} else if (c == ifNever) {
				// Nothing.
			} else {
				byte op = 0x80 + condOp(c);
				const Operand &src = instr->dest();
				switch (src.type()) {
				case opLabel:
					to->putByte(0x0F);
					to->putByte(op);
					to->putRelative(src.label());
					break;
				default:
					assert(false, L"Conditional jumps only support labels.");
					break;
				}
			}
		}

		void callOut(Output *to, Instr *instr) {
			jmpCall(to, true, instr->src());
		}

		void retOut(Output *to, Instr *instr) {
			to->putByte(0xC3);
		}

		void datOut(Output *to, Instr *instr) {
			Operand src = instr->src();
			switch (src.type()) {
			case opLabel:
				to->putAddress(src.label());
				break;
			case opReference:
				to->putAddress(src.ref());
				break;
			case opObjReference:
				to->putObject(src.object());
				break;
			case opConstant:
				to->putSize(src.constant(), src.size());
				break;
			default:
				assert(false, L"Unsupported type for 'dat'.");
				break;
			}
		}

#define OUTPUT(x) { op::x, &x ## Out }

		typedef void (*OutputFn)(Output *to, Instr *instr);

		const OpEntry<OutputFn> outputMap[] = {
			OUTPUT(push),
			OUTPUT(pop),
			OUTPUT(mov),
			OUTPUT(add),
			OUTPUT(adc),
			OUTPUT(bor),
			OUTPUT(band),
			OUTPUT(bnot),
			OUTPUT(sub),
			OUTPUT(sbb),
			OUTPUT(bxor),
			OUTPUT(cmp),
			OUTPUT(lea),
			OUTPUT(jmp),
			OUTPUT(call),
			OUTPUT(ret),

			OUTPUT(dat),
		};

		void output(Listing *src, Output *to) {
			static OpTable<OutputFn> t(outputMap, ARRAY_COUNT(outputMap));

			for (Nat i = 0; i < src->count(); i++) {
				to->mark(src->labels(i));

				Instr *instr = src->at(i);
				OutputFn fn = t[instr->op()];
				if (fn) {
					(*fn)(to, instr);
				} else {
					assert(false, L"Unsupported op-code: " + String(name(instr->op())));
				}
			}

			to->mark(src->labels(src->count()));
		}

	}
}
