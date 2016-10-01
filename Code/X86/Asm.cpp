#include "stdafx.h"
#include "Asm.h"
#include "Operand.h"
#include "Arena.h"
#include "Listing.h"

namespace code {
	namespace x86 {

		const Register ptrD = Register(0x010);
		const Register ptrSi = Register(0x011);
		const Register ptrDi = Register(0x012);
		const Register edx = Register(0x410);
		const Register esi = Register(0x411);
		const Register edi = Register(0x412);

		const wchar *nameX86(Register r) {
			switch (r) {
			case ptrD:
				return L"ptrD";
			case ptrSi:
				return L"ptrSi";
			case ptrDi:
				return L"ptrDi";

			case edx:
				return L"edx";
			case esi:
				return L"esi";
			case edi:
				return L"edi";
			}
			return null;
		}

		Register unusedReg(RegSet *in) {
			Register candidates[] = { ptrD, ptrA, ptrB, ptrC, ptrSi, ptrDi };
			Register protect64[] = { rax, noReg, noReg, noReg, rbx, rcx };
			for (nat i = 0; i < ARRAY_COUNT(candidates); i++) {
				if (!in->has(candidates[i])) {
					// See if we need to protect 64-bit registers...
					Register prot = protect64[i];
					if (prot == noReg)
						return candidates[i];

					// Either too small or not in the set?
					if (in->get(prot) != prot)
						return candidates[i];
				}
			}

			return noReg;
		}

		void add64(RegSet *to) {
			for (RegSet::Iter i = to->begin(); i != to->end(); ++i) {
				Register r = high32(*i);
				if (r != noReg)
					to->put(r);
			}
		}

		Register low32(Register reg) {
			if (size(reg) == Size::sLong)
				return asSize(reg, Size::sInt);
			else
				return reg;
		}

		Register high32(Register reg) {
			switch (reg) {
			case rax:
				return edx;
			case rbx:
				return esi;
			case rcx:
				return edi;
			default:
				return noReg;
			}
		}

		Operand low32(Operand o) {
			assert(o.size() == Size::sLong);
			switch (o.type()) {
			case opConstant:
				return natConst(o.constant() & 0xFFFFFFFF);
			case opRegister:
				return Operand(low32(o.reg()));
			case opRelative:
				return intRel(o.reg(), o.offset());
			case opVariable:
				return intRel(o.variable(), o.offset());
			}
			assert(false);
			return Operand();
		}

		Operand high32(Operand o) {
			assert(o.size() == Size::sLong);
			switch (o.type()) {
			case opConstant:
				return natConst(o.constant() >> 32);
			case opRegister:
				return Operand(high32(o.reg()));
			case opRelative:
				return intRel(o.reg(), o.offset() + Offset(4));
			case opVariable:
				return intRel(o.variable(), o.offset() + Offset(4));
			}
			assert(false);
			return Operand();
		}

		RegSet *allRegs(EnginePtr e) {
			RegSet *r = new (e.v) RegSet();
			r->put(eax);
			r->put(ebx);
			r->put(ecx);
			r->put(edx);
			r->put(esi);
			r->put(edi);
			return r;
		}

		RegSet *fnDirtyRegs(EnginePtr e) {
			RegSet *r = new (e.v) RegSet();
			r->put(eax);
			r->put(ecx);
			r->put(edx);
			return r;
		}

		Register preserve(Register r, RegSet *used, Listing *dest) {
			Engine &e = dest->engine();
			Register into = unusedReg(used);
			if (into == noReg) {
				*dest << push(r);
			} else {
				into = asSize(into, size(r));
				*dest << mov(into, r);
			}
			return into;
		}

		void restore(Register r, Register saved, Listing *dest) {
			Engine &e = dest->engine();

			if (saved == noReg) {
				*dest << pop(r);
			} else {
				*dest << mov(r, saved);
			}
		}

		Preserve::Preserve(RegSet *regs, RegSet *used, Listing *dest) {
			this->dest = dest;
			srcReg = new (dest) Array<Nat>();
			destReg = new (dest) Array<Nat>();
			RegSet *usedBefore = new (used) RegSet(used);
			RegSet *usedAfter = new (used) RegSet(used);
			// Do not attempt to use any of the registers we want to preserve to store stuff in.
			usedAfter->put(regs);

			add64(usedBefore);
			add64(usedAfter);

			for (RegSet::Iter i = regs->begin(); i != regs->end(); ++i) {
				if (usedBefore->has(*i)) {
					Register r = preserve(*i, usedAfter, dest);
					srcReg->push(Nat(*i));
					destReg->push(Nat(r));
					if (r != noReg)
						usedAfter->put(r);
				}
			}
		}

		void Preserve::restore() {
			for (Nat i = srcReg->count(); i > 0; i--) {
				Register src = Register(srcReg->at(i - 1));
				Register dest = Register(destReg->at(i - 1));
				code::x86::restore(src, dest, this->dest);
			}
		}

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

		byte condOp(CondFlag c) {
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
			case ifFBelow:
				return 0x2;
			case ifBelowEqual:
			case ifFBelowEqual:
				return 0x6;
			case ifAboveEqual:
			case ifFAboveEqual:
				return 0x3;
			case ifAbove:
			case ifFAbove:
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

			assert(false, L"Missing jmpCond " + String(name(c)));
			return 0;
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
				to->putByte(op.opImm32);
				modRm(to, op.modeImm32, dest);
				to->putAddress(src.label());
				break;
			case opReference:
				to->putByte(op.opImm32);
				modRm(to, op.modeImm32, dest);
				to->putAddress(src.ref());
				break;
			case opObjReference:
				to->putByte(op.opImm32);
				modRm(to, op.modeImm32, dest);
				to->putObject(src.object());
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