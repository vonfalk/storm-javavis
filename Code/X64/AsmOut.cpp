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
				TODO(L"FIXME");
				break;
			case opReference:
				to->putByte(0xFF);
				to->putByte(0x35); // 6/[RIP+disp32]
				to->putObjRelative(src.ref());
				break;
			case opObjReference:
				to->putByte(0xFF);
				to->putByte(0x35); // 6/[RIP+disp32]
				to->putObjRelative(src.object());
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
				TODO(L"FIXME");
				break;
			default:
				assert(false, L"Pop does not support this operand type.");
				break;
			}
		}

		void movOut(Output *to, Instr *instr) {
			// TODO!
		}

		void jmpOut(Output *to, Instr *instr) {
			// TODO!
		}

		void callOut(Output *to, Instr *instr) {
			// TODO!
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
