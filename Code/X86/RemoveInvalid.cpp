#include "stdafx.h"
#include "RemoveInvalid.h"
#include "Listing.h"
#include "Arena.h"

namespace code {
	namespace x86 {

#define IMM_REG(x) { op::x, &RemoveInvalid::immRegTfm }
#define TRANSFORM(x) { op::x, &RemoveInvalid::x ## Tfm }

		const OpEntry<RemoveInvalid::TransformFn> RemoveInvalid::transformMap[] = {
			IMM_REG(mov),
			IMM_REG(add),
			IMM_REG(adc),
			IMM_REG(or),
			IMM_REG(and),
			IMM_REG(sub),
			IMM_REG(sbb),
			IMM_REG(xor),
			IMM_REG(cmp),

			TRANSFORM(lea),
			TRANSFORM(mul),
			TRANSFORM(idiv),
			TRANSFORM(udiv),
			TRANSFORM(imod),
			TRANSFORM(umod),
			TRANSFORM(setCond),
			TRANSFORM(shl),
			TRANSFORM(shr),
			TRANSFORM(sar),
			TRANSFORM(icast),
			TRANSFORM(ucast),

			TRANSFORM(callFloat),
			TRANSFORM(retFloat),
			TRANSFORM(fnCallFloat),

			TRANSFORM(fnParam),
			TRANSFORM(fnParamRef),
			TRANSFORM(fnCall),
		};

		RemoveInvalid::RemoveInvalid() {}

		void RemoveInvalid::before(Listing *dest, Listing *src) {
			used = usedRegisters(src).used;

			// Add 64-bit aliases everywhere.
			for (nat i = 0; i < used->count(); i++)
				add64(used->at(i));
		}

		void RemoveInvalid::during(Listing *dest, Listing *src, Nat line) {
			static OpTable<TransformFn> t(transformMap, ARRAY_COUNT(transformMap));

			Instr *i = src->at(line);
			TransformFn f = t[i->op()];
			if (f) {
				(this->*f)(dest, src, line);
			} else {
				*dest << i;
			}
		}

		Register RemoveInvalid::unusedReg(Nat line) {
			return code::x86::unusedReg(used->at(line));
		}

		// ImmReg combination already supported?
		static bool supported(Instr *instr) {
			switch (instr->src().type()) {
			case opLabel:
			case opReference:
			case opConstant:
			case opRegister:
				return true;
			default:
				if (instr->dest().type() == opRegister)
					return true;
				break;
			}

			return false;
		}

		void RemoveInvalid::immRegTfm(Listing *dest, Listing *src, Nat line) {
			Instr *instr = src->at(line);

			if (supported(instr)) {
				*dest << instr;
				return;
			}

			Size size = instr->src().size();
			assert(size <= Size::sInt, "The 64-bit transform should have fixed this!");

			Register reg = unusedReg(line);
			Engine &e = engine();
			if (reg == noReg) {
				reg = asSize(ptrD, size);
				*dest << code::push(e, ptrD);
				*dest << code::mov(e, reg, instr->src());
				*dest << instr->alterSrc(reg);
				*dest << code::pop(e, ptrD);
			} else {
				reg = asSize(reg, size);
				*dest << code::mov(e, reg, instr->src());
				*dest << instr->alterSrc(reg);
			}
		}

		void RemoveInvalid::leaTfm(Listing *dest, Listing *src, Nat line) {
			Instr *instr = src->at(line);

			// We can encode writing directly to a register.
			if (instr->dest().type() == opRegister) {
				*dest << instr;
				return;
			}

			Register reg = unusedReg(line);
			Engine &e = engine();
			if (reg == noReg) {
				*dest << code::push(e, ptrD);
				*dest << code::lea(e, ptrD, instr->src());
				*dest << code::mov(e, instr->dest(), ptrD);
				*dest << code::pop(e, ptrD);
			} else {
				reg = asSize(reg, Size::sPtr);
				*dest << code::lea(e, reg, instr->src());
				*dest << code::mov(e, instr->dest(), reg);
			}
		}

		void RemoveInvalid::mulTfm(Listing *dest, Listing *src, Nat line) {
			Instr *instr = src->at(line);

			Size size = instr->size();
			assert(size != Size::sByte && size <= Size::sInt, "Bytes not supported yet!");

			if (instr->dest().type() == opRegister) {
				*dest << instr;
				return;
			}

			// Only supported mmode is mul <reg>, <r/m>. Move dest into a register.
			Engine &e = engine();
			Register reg = unusedReg(line);
			if (reg == noReg) {
				reg = asSize(ptrD, size);
				*dest << code::push(e, ptrD);
				*dest << code::mov(e, reg, instr->dest());
				*dest << instr->alterDest(reg);
				*dest << code::mov(e, instr->dest(), reg);
				*dest << code::pop(e, ptrD);
			} else {
				reg = asSize(reg, size);
				*dest << code::mov(e, reg, instr->dest());
				*dest << instr->alterDest(reg);
				*dest << code::mov(e, instr->dest(), reg);
			}
		}

		void RemoveInvalid::idivTfm(Listing *to, Listing *src, Nat line) {
			Instr *instr = src->at(line);

			Engine &e = engine();
			Operand dest = instr->dest();
			bool srcConst = instr->src().type() == opConstant;
			bool destEax = false;

			if (dest.type() == opRegister && same(dest.reg(), ptrA)) {
				destEax = true;

				if (!srcConst) {
					// Supported!
					*to << instr;
					return;
				}
			}

			// The 64-bit transform has been executed before, so we are sure that size is <= sInt
			bool isByte = dest.size() == Size::sByte;
			Operand newSrc = instr->src();

			RegSet *used = this->used->at(line);

			// Clear eax and edx.
			if (!destEax && used->has(eax))
				*to << push(e, eax);
			if (!isByte && used->has(edx))
				*to << push(e, edx);

			// Move dest into eax first.
			*to << mov(e, eax, dest);

			if (srcConst) {
				if (used->has(ebx))
					*to << push(e, ebx);
				*to << mov(e, ebx, instr->src());
				newSrc = ebx;
			}

			// Clear edx.
			*to << xor(e, edx, edx);
			*to << instr->alter(eax, newSrc);
			*to << mov(e, dest, eax);

			if (srcConst && used->has(ebx))
				*to << pop(e, ebx);
			if (!isByte && used->has(edx))
				*to << pop(e, edx);
			if (!destEax && used->has(eax))
				*to << pop(e, eax);
		}

		void RemoveInvalid::udivTfm(Listing *dest, Listing *src, Nat line) {
			idivTfm(dest, src, line);
		}

		void RemoveInvalid::imodTfm(Listing *to, Listing *src, Nat line) {
			Instr *instr = src->at(line);

			Engine &e = engine();
			Operand dest = instr->dest();
			bool srcConst = instr->src().type() == opConstant;
			bool eaxDest = dest.type() == opRegister && same(dest.reg(), ptrA);
			bool isByte = dest.size() == Size::sByte;

			Operand newSrc = instr->src();
			RegSet *used = this->used->at(line);

			// Clear eax and edx if needed.
			if (!eaxDest && used->has(eax))
				*to << push(e, eax);
			if (!isByte && used->has(edx))
				*to << push(e, edx);

			// Move source into eax.
			*to << mov(e, eax, dest);

			if (srcConst) {
				if (used->has(ebx))
					*to << push(e, ebx);
				*to << mov(e, ebx, instr->src());
				newSrc = ebx;
			}

			// Clear edx.
			*to << xor(e, edx, edx);
			*to << instr->alter(eax, newSrc);
			*to << mov(e, dest, edx);

			if (srcConst && used->has(ebx))
				*to << pop(e, ebx);
			if (!isByte && used->has(edx))
				*to << pop(e, edx);
			if (!eaxDest && used->has(eax))
				*to << pop(e, eax);
		}

		void RemoveInvalid::umodTfm(Listing *dest, Listing *src, Nat line) {
			imodTfm(dest, src, line);
		}

		void RemoveInvalid::setCondTfm(Listing *dest, Listing *src, Nat line) {
			Instr *instr = src->at(line);

			switch (instr->src().condFlag()) {
			case ifAlways:
				*dest << mov(engine(), instr->dest(), byteConst(1));
				break;
			case ifNever:
				*dest << mov(engine(), instr->dest(), byteConst(0));
				break;
			default:
				*dest << instr;
				break;
			}
		}

		void RemoveInvalid::shlTfm(Listing *dest, Listing *src, Nat line) {
			Instr *instr = src->at(line);
			Engine &e = engine();

			switch (instr->src().type()) {
			case opRegister:
				if (instr->src().reg() == cl) {
					*dest << instr;
					return;
				}
				break;
			case opConstant:
				// Supported!
				*dest << instr;
				return;
			}

			Size size = instr->dest().size();

			// We need to store the value in cl. See if dest is also cl or ecx:
			if (instr->dest().type() == opRegister && same(instr->dest().reg(), ecx)) {
				// Yup. We need to swap things around a lot!
				Register reg = asSize(unusedReg(line), size);

				if (reg == noReg) {
					// Ugh... Worst case!
					*dest << push(e, ecx);
					*dest << mov(e, cl, instr->src());
					*dest << instr->alter(xRel(size, ptrStack, Offset(0)), cl);
					*dest << pop(e, ecx);
				} else {
					*dest << mov(e, reg, instr->dest());
					*dest << mov(e, cl, instr->src());
					*dest << instr->alter(reg, cl);
					*dest << mov(e, instr->dest(), reg);
				}
			} else {
				// We have a bit more leeway at least!
				Register reg = asSize(unusedReg(line), Size::sInt);

				if (reg == noReg) {
					*dest << push(e, ecx);
					*dest << mov(e, cl, instr->src());
					*dest << instr->alterSrc(cl);
					*dest << pop(e, ecx);
				} else {
					*dest << mov(e, reg, ecx);
					*dest << mov(e, cl, instr->src());
					*dest << instr->alterSrc(cl);
					*dest << mov(e, ecx, reg);
				}
			}
		}

		void RemoveInvalid::shrTfm(Listing *dest, Listing *src, Nat line) {
			shlTfm(dest, src, line);
		}

		void RemoveInvalid::sarTfm(Listing *dest, Listing *src, Nat line) {
			shlTfm(dest, src, line);
		}

		void RemoveInvalid::icastTfm(Listing *dest, Listing *src, Nat line) {
			Instr *instr = src->at(line);
			Size sFrom = instr->src().size();
			Size sTo = instr->dest().size();
			Engine &e = engine();

			if (instr->dest() == Operand(asSize(eax, sTo))) {
				*dest << instr;
				return;
			}

			bool toEax = instr->dest().type() != opRegister || !same(instr->dest().reg(), eax);

			RegSet *used = this->used->at(line);
			bool saveEax = used->has(eax);
			bool saveEdx = used->has(edx);

			if (toEax)
				saveEax = false;
			if (sFrom != Size::sLong && sTo != Size::sLong)
				saveEdx = false;

			if (saveEax)
				*dest << push(e, eax);
			if (saveEdx)
				*dest << push(e, edx);

			if ((sFrom == Size::sByte && sTo == Size::sLong) ||
				(sFrom == Size::sLong && sTo == Size::sByte)) {
				*dest << instr->alterDest(eax);
				*dest << instr->alter(asSize(eax, sTo), eax);
			} else {
				*dest << instr->alterDest(asSize(eax, sTo));
			}

			if (!toEax) {
				if (sTo == Size::sLong) {
					*dest << mov(e, low32(instr->dest()), eax);
					*dest << mov(e, high32(instr->dest()), edx);
				} else {
					*dest << mov(e, instr->dest(), asSize(eax, sTo));
				}
			}

			if (saveEdx)
				*dest << pop(e, edx);
			if (saveEax)
				*dest << pop(e, edx);
		}

		void RemoveInvalid::ucastTfm(Listing *dest, Listing *src, Nat line) {
			icastTfm(dest, src, line);
		}

		void RemoveInvalid::callFloatTfm(Listing *dest, Listing *src, Nat line) {
			Instr *instr = src->at(line);
			Size s = instr->src().size();
			Engine &e = engine();

			*dest << call(e, instr->src(), ValType(s, false));
			*dest << sub(e, ptrStack, ptrConst(s));
			*dest << fstp(e, xRel(s, ptrStack, Offset()));
			*dest << pop(e, instr->dest());
		}

		void RemoveInvalid::retFloatTfm(Listing *dest, Listing *src, Nat line) {
			Instr *instr = src->at(line);
			Size s = instr->src().size();
			Engine &e = engine();

			*dest << push(e, instr->src());
			*dest << fld(e, xRel(s, ptrStack, Offset()));
			*dest << add(e, ptrStack, ptrConst(s));
			*dest << ret(e, ValType(s, false));
		}

		void RemoveInvalid::fnCallFloatTfm(Listing *dest, Listing *src, Nat line) {
			Instr *instr = src->at(line);
			Size s = instr->src().size();
			Engine &e = engine();

			*dest << fnCall(e, instr->src(), ValType(s, false));
			*dest << sub(e, ptrStack, ptrConst(s));
			*dest << fstp(e, xRel(s, ptrStack, Offset()));
			*dest << pop(e, instr->dest());
		}

		void RemoveInvalid::fnParamTfm(Listing *dest, Listing *src, Nat line) {
			TODO(L"FIXME!");
		}

		void RemoveInvalid::fnParamRefTfm(Listing *dest, Listing *src, Nat line) {
			TODO(L"FIXME!");
		}

		void RemoveInvalid::fnCallTfm(Listing *dest, Listing *src, Nat line) {
			TODO(L"FIXME!");
		}

	}
}
