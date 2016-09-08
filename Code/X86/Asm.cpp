#include "stdafx.h"
#include "Asm.h"
#include "Operand.h"
#include "Arena.h"

namespace code {
	namespace x86 {

		nat registerId(Register r) {
			switch (r) {
			case al:
			case ptrA:
			case eax:
				return 0;
			case cl:
			case ptrC:
			case ecx:
				return 1;
			case bl:
			case ptrB:
			case ebx:
				return 3;

			case ptrStack:
				return 4;
			case ptrFrame:
				return 5;
			default:
				if (r == ptrD || r == edx)
					return 2;
				if (r == ptrSi || r == esi)
					return 6;
				if (r == ptrDi || r == edi)
					return 7;
				assert(false);
				return 0;
			}
		}

		bool singleByte(Word value) {
			Long v(value);
			return (v >= -128 && v <= 127);
		}

		byte sibValue(byte baseReg, byte scaledReg, byte scale) {
			assert(baseReg != 5);
			assert(scaledReg != 4);
			if (scaledReg == 0xFF)
				scaledReg = 4; // no scaling
			assert(scaledReg != 4 || scale == 1);
			assert(scale <= 4);
			static const byte scaleMap[9] = { -1, 0, 1, -1, 2, -1, -1, -1, 3 };
			scale = scaleMap[scale];
			assert(scale < 4 && baseReg < 8 && scaledReg < 8);
			return (scale << 6) | (scaledReg << 3) | baseReg;
		}

		byte modRmValue(byte mode, byte src, byte dest) {
			assert(mode < 4 && src < 8 && dest < 8);
			return (mode << 6) | (src << 3) | dest;
		}

		void modRm(Output *to, byte subOp, const Operand &dest) {
			switch (dest.type()) {
			case opRegister:
				to->putByte(modRmValue(3, subOp, registerId(dest.reg())));
				break;
			case opRelative:
				if (dest.reg() == noReg) {
					to->putByte(modRmValue(0, subOp, 5));
					to->putInt(dest.offset().current());
				} else {
					byte mode = 2;
					nat reg = registerId(dest.reg());

					if (dest.offset() == Offset(0)) {
						if (reg == 5) {
							// We need to used disp8 for ebp...
							mode = 1;
						} else {
							mode = 0;
						}
					} else if (singleByte(dest.offset().current())) {
						mode = 1;
					}

					to->putByte(modRmValue(mode, subOp, reg));
					if (reg == 4) {
						// SIB-byte for reg=ESP!
						to->putByte(sibValue(reg));
					}

					if (mode == 1) {
						to->putByte(byte(dest.offset().current()));
					} else if (mode == 2) {
						to->putInt(Nat(dest.offset().current()));
					}
				}
				break;
			default:
				// Not implemented yet, there are many more!
				assert(false, L"This modRm mode is not implemented yet.");
				break;
			}
		}

		void immRegInstr(Output *to, const ImmRegInstr &op, const Operand &dest, const Operand &src) {
			switch (src.type()) {
			case opLabel:
				TODO(L"Memory moves, so absolute label addresses are a bad idea without proper care.");
				NOT_DONE;
				// to->putByte(op.opImm32);
				// modRm(to, op.modeImm32, dest);
				// to->putAddress(src.label());
				break;
			case opReference:
				NOT_DONE;
				// to->putByte(op.opImm32);
				// modRm(to, op.modeImm32, dest);
				// to->putAddress(src.reference());
				break;
			case opConstant:
				if (op.modeImm8 != 0xFF && singleByte(src.constant())) {
					to->putByte(op.opImm8);
					modRm(to, op.modeImm8, dest);
					to->putByte(src.constant() & 0xFF);
				} else {
					to->putByte(op.opImm32);
					modRm(to, op.modeImm32, dest);
					to->putInt(Nat(src.constant()));
				}
				break;
			case opRegister:
				to->putByte(op.opSrcReg);
				modRm(to, registerId(src.reg()), dest);
				break;
			default:
				if (dest.type() == opRegister) {
					to->putByte(op.opDestReg);
					modRm(to, registerId(dest.reg()), src);
				} else {
					assert(false); // This mode is _not_ supported.
				}
				break;
			}
		}

		void immRegInstr(Output *to, const ImmRegInstr8 &op, const Operand &dest, const Operand &src) {
			switch (src.type()) {
			case opConstant:
				to->putByte(op.opImm);
				modRm(to, op.modeImm, dest);
				to->putByte(src.constant() & 0xFF);
				break;
			case opRegister:
				to->putByte(op.opSrcReg);
				modRm(to, registerId(src.reg()), dest);
				break;
			default:
				if (dest.type() == opRegister) {
					to->putByte(op.opDestReg);
					modRm(to, registerId(dest.reg()), src);
				} else {
					assert(false); // not supported on x86
				}
				break;
			}
		}

		void immRegInstr(Output *to, const ImmRegInstr8 &op8, const ImmRegInstr &op, const Operand &dest, const Operand &src) {
			Size size = src.size();
			if (size == Size::sInt || size == Size::sPtr) {
				immRegInstr(to, op, dest, src);
			} else if (size == Size::sByte) {
				immRegInstr(to, op8, dest, src);
			} else {
				assert(false, L"Fail: " + ::toS(size));
			}
		}

	}
}
