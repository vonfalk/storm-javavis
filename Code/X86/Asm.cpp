#include "stdafx.h"
#include "Asm.h"
#include "Operand.h"
#include "Arena.h"
#include "Listing.h"
#include "Exception.h"

namespace code {
	namespace x86 {

		const Reg ptrD = Reg(0x010);
		const Reg ptrSi = Reg(0x011);
		const Reg ptrDi = Reg(0x012);
		const Reg dl = Reg(0x110);
		const Reg sil = Reg(0x111);
		const Reg dil = Reg(0x112);
		const Reg edx = Reg(0x410);
		const Reg esi = Reg(0x411);
		const Reg edi = Reg(0x412);
		const Reg rdx = Reg(0x810);
		const Reg rsi = Reg(0x811);
		const Reg rdi = Reg(0x812);

		const wchar *nameX86(Reg r) {
			switch (r) {
			case ptrD:
				return S("ptrD");
			case ptrSi:
				return S("ptrSi");
			case ptrDi:
				return S("ptrDi");

			case dl:
				return S("dl");
			case sil:
				return S("sil");
			case dil:
				return S("dil");

			case edx:
				return S("edx");
			case esi:
				return S("esi");
			case edi:
				return S("edi");

			case rdx:
				return S("rdx");
			case rsi:
				return S("rsi");
			case rdi:
				return S("rdi");
			default:
				return null;
			}
		}

		Reg unusedReg(RegSet *in) {
			static const Reg candidates[] = { ptrD, ptrA, ptrB, ptrC, ptrSi, ptrDi };
			static const Reg protect64[] = { rax, noReg, noReg, noReg, rbx, rcx };
			for (nat i = 0; i < ARRAY_COUNT(candidates); i++) {
				if (!in->has(candidates[i])) {
					// See if we need to protect 64-bit registers...
					Reg prot = protect64[i];
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
				Reg r = high32(*i);
				if (r != noReg)
					to->put(r);
			}
		}

		Reg low32(Reg reg) {
			if (size(reg) == Size::sLong)
				return asSize(reg, Size::sInt);
			else
				return reg;
		}

		Reg high32(Reg reg) {
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
			case opRelativeRef:
				return intRel(o.reg(), o.ref(), o.offset());
			case opVariable:
				return intRel(o.var(), o.offset());
			case opVariableRef:
				return intRel(o.var(), o.ref(), o.offset());
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
			case opRelativeRef:
				return intRel(o.reg(), o.ref(), o.offset() + Offset(4));
			case opVariable:
				return intRel(o.var(), o.offset() + Offset(4));
			case opVariableRef:
				return intRel(o.var(), o.ref(), o.offset() + Offset(4));
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

		Reg preserve(Reg r, RegSet *used, Listing *dest) {
			Reg into = unusedReg(used);
			if (into == noReg) {
				*dest << push(r);
			} else {
				into = asSize(into, size(r));
				*dest << mov(into, r);
			}
			return into;
		}

		void restore(Reg r, Reg saved, Listing *dest) {
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
			RegSet *usedBefore = new (used) RegSet(*used);
			RegSet *usedAfter = new (used) RegSet(*used);
			// Do not attempt to use any of the registers we want to preserve to store stuff in.
			usedAfter->put(regs);

			add64(usedBefore);
			add64(usedAfter);

			for (RegSet::Iter i = regs->begin(); i != regs->end(); ++i) {
				if (usedBefore->has(*i)) {
					Reg r = preserve(*i, usedAfter, dest);
					srcReg->push(Nat(*i));
					destReg->push(Nat(r));
					if (r != noReg)
						usedAfter->put(r);
				}
			}
		}

		void Preserve::restore() {
			for (Nat i = srcReg->count(); i > 0; i--) {
				Reg src = Reg(srcReg->at(i - 1));
				Reg dest = Reg(destReg->at(i - 1));
				code::x86::restore(src, dest, this->dest);
			}
		}

		nat registerId(Reg r) {
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
				if (asSize(r, Size::sPtr) == ptrD)
					return 2;
				if (asSize(r, Size::sPtr) == ptrSi)
					return 6;
				if (asSize(r, Size::sPtr) == ptrDi)
					return 7;
				assert(false, L"Can not use " + ::toS(name(r)));
				return 0;
			}
		}

		byte condOp(CondFlag c) {
			switch (c) {
			case ifAlways:
			case ifNever:
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
			return v >= -128 && v <= 127;
		}

		byte sibValue(byte baseReg, byte scaledReg, byte scale) {
			assert(baseReg != 5);
			assert(scaledReg != 4);
			if (scaledReg == 0xFF)
				scaledReg = 4; // no scaling
			assert(scaledReg != 4 || scale == 1);
			assert(scale <= 4);
			static const byte scaleMap[9] = { 0xFF, 0, 1, 0xFF, 2, 0xFF, 0xFF, 0xFF, 3 };
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
					to->putInt(dest.offset().v32());
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
					} else if (singleByte(dest.offset().v32())) {
						mode = 1;
					}

					to->putByte(modRmValue(mode, subOp, reg));
					if (reg == 4) {
						// SIB-byte for reg=ESP!
						to->putByte(sibValue(reg));
					}

					if (mode == 1) {
						to->putByte(byte(dest.offset().v32()));
					} else if (mode == 2) {
						to->putInt(Nat(dest.offset().v32()));
					}
				}
				break;
			case opRelativeRef:
				if (dest.reg() == noReg) {
					to->putByte(modRmValue(0, subOp, 5));
					to->putOffset(dest.ref(), dest.offset().v32());
				} else {
					nat reg = registerId(dest.reg());
					to->putByte(modRmValue(2, subOp, reg));
					if (reg == 4) {
						// SIB-byte for reg=ESP!
						to->putByte(sibValue(reg));
					}

					to->putOffset(dest.ref(), dest.offset().v32());
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
					assert(false, L"ModRM mode not supported.");
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
				assert(false, L"Operand size not supported: " + ::toS(size));
			}
		}

		bool resultParam(TypeDesc *desc) {
			return as<PrimitiveDesc>(desc) == null;
		}

		Nat encodeFnState(Nat block, Nat activation) {
			if (block != Block().key() && block > 0xFFFE)
				throw new (runtime::someEngine()) InvalidValue(S("The X86 backend does not support more than 65535 blocks."));
			if (activation > 0xFFFF)
				throw new (runtime::someEngine()) InvalidValue(S("The X86 backend does not support more than 65536 activations."));

			if (block == Block().key())
				block = 0xFFFF;

			return (block << 16) | activation;
		}

		void decodeFnState(Nat original, Nat &block, Nat &activation) {
			block = (original >> 16) & 0xFFFF;
			activation = original & 0xFFFF;

			if (block == 0xFFFF)
				block = Block().key();
		}

	}
}
