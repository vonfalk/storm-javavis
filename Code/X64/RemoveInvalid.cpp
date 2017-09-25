#include "stdafx.h"
#include "RemoveInvalid.h"
#include "Asm.h"
#include "../Listing.h"
#include "../UsedRegs.h"

namespace code {
	namespace x64 {

#define TRANSFORM(x) { op::x, &RemoveInvalid::x ## Tfm }
#define IMM_REG(x) { op::x, &RemoveInvalid::immRegTfm }
#define DEST_W_REG(x) { op::x, &RemoveInvalid::destRegWTfm }
#define DEST_RW_REG(x) { op::x, &RemoveInvalid::destRegRwTfm }

		const OpEntry<RemoveInvalid::TransformFn> RemoveInvalid::transformMap[] = {
			IMM_REG(mov),
			IMM_REG(add),
			IMM_REG(adc),
			IMM_REG(bor),
			IMM_REG(band),
			IMM_REG(sub),
			IMM_REG(sbb),
			IMM_REG(bxor),
			IMM_REG(cmp),

			DEST_W_REG(lea),
			DEST_W_REG(icast),
			DEST_W_REG(ucast),
			DEST_RW_REG(mul),

			TRANSFORM(fnCall),
			TRANSFORM(fnParam),
			TRANSFORM(fnParamRef),
			TRANSFORM(fnRet),

			TRANSFORM(idiv),
			TRANSFORM(udiv),
			TRANSFORM(imod),
			TRANSFORM(umod),
		};

		RemoveInvalid::RemoveInvalid() {}

		void RemoveInvalid::before(Listing *dest, Listing *src) {
			used = usedRegs(dest->arena, src).used;
			large = new (this) Array<Operand>();
			lblLarge = dest->label();
		}

		void RemoveInvalid::during(Listing *dest, Listing *src, Nat line) {
			static OpTable<TransformFn> t(transformMap, ARRAY_COUNT(transformMap));

			Instr *i = src->at(line);
			switch (i->op()) {
			case op::call:
			case op::fnCall:
			case op::fnCallFloat:
			case op::jmp:
				// Nothing needed. We deal with these later on in the chain.
				break;
			default:
				i = extractNumbers(i);
				i = extractComplex(dest, i, line);
				break;
			}

			TransformFn f = t[i->op()];
			if (f) {
				(this->*f)(dest, i, line);
			} else {
				*dest << i;
			}
		}

		void RemoveInvalid::after(Listing *dest, Listing *src) {
			// Output all constants.
			*dest << lblLarge;
			for (Nat i = 0; i < large->count(); i++) {
				*dest << dat(large->at(i));
			}
		}

		Instr *RemoveInvalid::extractNumbers(Instr *i) {
			Operand src = i->src();
			if (src.type() == opConstant && src.size() == Size::sWord && !singleInt(src.constant())) {
				i = i->alterSrc(longRel(lblLarge, Offset::sWord*large->count()));
				large->push(src);
			}

			// Labels are also constants.
			if (src.type() == opLabel) {
				i = i->alterSrc(ptrRel(lblLarge, Offset::sWord*large->count()));
				large->push(src);
			}

			// Since writing to a constant is not allowed, we will not attempt to extract 'dest'.
			return i;
		}

		static bool isComplex(Listing *l, Operand op) {
			if (op.type() != opVariable)
				return false;

			Var v = op.var();
			TypeDesc *t = l->paramDesc(v);
			if (!t)
				return false;

			return as<ComplexDesc>(t) != null;
		}

		Instr *RemoveInvalid::extractComplex(Listing *to, Instr *i, Nat line) {
			// Complex parameters are passed as a pointer. Dereference these by inserting a 'mov' instruction.
			RegSet *regs = used->at(line);
			if (isComplex(to, i->src())) {
				Reg reg = asSize(unusedReg(regs), Size::sPtr);
				regs = new (this) RegSet(*regs);
				regs->put(reg);

				*to << mov(reg, i->src());
				i = i->alterSrc(ptrRel(reg, Offset()));
			}

			if (isComplex(to, i->dest())) {
				Reg reg = asSize(unusedReg(regs), Size::sPtr);
				*to << mov(reg, i->dest());
				i = i->alterDest(ptrRel(reg, Offset()));
			}

			return i;
		}

		// Is this src+dest combination supported for immReg op-codes?
		static bool supported(Instr *instr) {
			// Basically: one operand has to be a register, except when 'src' is a constant.
			switch (instr->src().type()) {
			case opConstant:
				// If a constant remains this far, it is small enough to be an immediate value!
			case opRegister:
				return true;
			default:
				if (instr->dest().type() == opRegister)
					return true;
				break;
			}

			return false;
		}

		void RemoveInvalid::immRegTfm(Listing *dest, Instr *instr, Nat line) {
			if (supported(instr)) {
				*dest << instr;
				return;
			}

			Size size = instr->src().size();
			Reg reg = unusedReg(used->at(line));
			reg = asSize(reg, size);
			*dest << mov(reg, instr->src());
			*dest << instr->alterSrc(reg);
		}

		void RemoveInvalid::destRegWTfm(Listing *dest, Instr *instr, Nat line) {
			if (instr->dest().type() == opRegister) {
				*dest << instr;
				return;
			}

			Reg reg = unusedReg(used->at(line));
			reg = asSize(reg, instr->dest().size());
			*dest << instr->alterDest(reg);
			*dest << mov(instr->dest(), reg);
		}

		void RemoveInvalid::destRegRwTfm(Listing *dest, Instr *instr, Nat line) {
			if (instr->dest().type() == opRegister) {
				*dest << instr;
				return;
			}

			Reg reg = unusedReg(used->at(line));
			reg = asSize(reg, instr->dest().size());
			*dest << mov(reg, instr->dest());
			*dest << instr->alterDest(reg);
			*dest << mov(instr->dest(), reg);
		}

		void RemoveInvalid::fnParamTfm(Listing *dest, Instr *instr, Nat line) {
			TODO(L"IMPLEMENT ME!");
		}

		void RemoveInvalid::fnParamRefTfm(Listing *dest, Instr *instr, Nat line) {
			TODO(L"IMPLEMENT ME!");
		}

		void RemoveInvalid::fnCallTfm(Listing *dest, Instr *instr, Nat line) {
			TODO(L"IMPLEMENT ME!");
		}

		void RemoveInvalid::fnRetTfm(Listing *dest, Instr *instr, Nat line) {
			TODO(L"IMPLEMENT ME!");
		}

		void RemoveInvalid::shlTfm(Listing *dest, Instr *instr, Nat line) {
			switch (instr->src().type()) {
			case opRegister:
				if (instr->src().reg() == cl) {
					*dest << instr;
					return;
				}
				break;
			case opConstant:
				*dest << instr;
				return;
			}

			Size size = instr->dest().size();

			// We need to store the value in 'cl'. See if 'dest' is also 'cl' or 'rcx'.
			if (instr->dest().type() == opRegister && same(instr->dest().reg(), rcx)) {
				// Yes. We need to swap things around quite a lot!
				Reg reg = asSize(unusedReg(used->at(line)), size);
				*dest << mov(reg, instr->dest());
				*dest << mov(cl, instr->src());
				*dest << instr->alter(reg, cl);
				*dest << mov(instr->dest(), reg);
			} else {
				// We do not need to do that at least!
				Reg reg = asSize(unusedReg(used->at(line)), Size::sLong);
				*dest << mov(reg, rcx);
				*dest << mov(cl, instr->src());
				*dest << instr->alterSrc(cl);
				*dest << mov(rcx, reg);
			}
		}

		void RemoveInvalid::shrTfm(Listing *dest, Instr *instr, Nat line) {
			shlTfm(dest, instr, line);
		}

		void RemoveInvalid::sarTfm(Listing *dest, Instr *instr, Nat line) {
			shlTfm(dest, instr, line);
		}

		void RemoveInvalid::idivTfm(Listing *dest, Instr *instr, Nat line) {
			RegSet *used = new (this) RegSet(*this->used->at(line));
			const Operand &op = instr->dest();
			bool small = op.size() == Size::sByte;

			// If 'src' is a constant, we need to move it into a register.
			if (instr->src().type() == opConstant) {
				Reg r = asSize(unusedReg(used), instr->src().size());
				*dest << mov(r, instr->src());
				instr = instr->alterSrc(r);
			}

			// Make sure ptrD can be trashed (not needed if we're working with bytes).
			Reg oldD = noReg;
			if (!small && used->has(ptrD)) {
				oldD = asSize(unusedReg(used), Size::sPtr);
				*dest << mov(oldD, ptrD);
				used->put(oldD);
			}

			if (op.type() == opRegister && same(op.reg(), ptrA)) {
				// Supported!
				*dest << instr;
			} else {
				// We need to put op into 'ptrA'.
				Reg oldA = noReg;
				if (used->has(ptrA)) {
					oldA = asSize(unusedReg(used), Size::sPtr);
					*dest << mov(oldA, ptrA);
					used->put(oldA);
				}

				Reg destA = asSize(ptrA, op.size());
				*dest << mov(destA, op);
				if (instr->src().type() == opRegister && same(instr->src().reg(), ptrA)) {
					*dest << instr->alter(destA, asSize(oldA, instr->src().size()));
				} else {
					*dest << instr->alterDest(destA);
				}
				*dest << mov(op, destA);

				if (oldA != noReg) {
					*dest << mov(ptrA, oldA);
				}
			}

			if (oldD != noReg) {
				*dest << mov(ptrD, oldD);
			}
		}

		void RemoveInvalid::udivTfm(Listing *dest, Instr *instr, Nat line) {
			idivTfm(dest, instr, line);
		}

		void RemoveInvalid::imodTfm(Listing *dest, Instr *instr, Nat line) {
			RegSet *used = new (this) RegSet(*this->used->at(line));
			const Operand &op = instr->dest();
			bool small = op.size() == Size::sByte;

			// If 'src' is a constant, we need to move it into a register.
			if (instr->src().type() == opConstant) {
				Reg r = asSize(unusedReg(used), instr->src().size());
				*dest << mov(r, instr->src());
				instr = instr->alterSrc(r);
			}

			// Make sure ptrD can be trashed (unless we're working with 8 bit numbers).
			Reg oldD = noReg;
			if (!small && used->has(ptrD)) {
				oldD = asSize(unusedReg(used), Size::sPtr);
				*dest << mov(oldD, ptrD);
				used->put(oldD);
			}

			// We need to put op into 'ptrA'.
			Reg oldA = noReg;
			if (used->has(ptrA)) {
				oldA = asSize(unusedReg(used), Size::sPtr);
				*dest << mov(oldA, ptrA);
				used->put(oldA);
			}

			Reg destA = asSize(ptrA, op.size());
			if (op.type() != opRegister || op.reg() != destA)
				*dest << mov(destA, op);

			if (instr->src().type() == opRegister && same(instr->src().reg(), ptrA)) {
				*dest << instr->alter(destA, asSize(oldA, instr->src().size()));
			} else {
				*dest << instr->alterDest(destA);
			}

			Reg destD = asSize(ptrD, op.size());
			if (small) {
				// We need to shift the register a bit (we are not able to access AH in this implementation).
				*dest << shr(eax, byteConst(8));
				destD = al;
			}

			if (op.type() != opRegister || op.reg() != destD)
				*dest << mov(op, destD);

			// Restore registers.
			if (oldA != noReg) {
				*dest << mov(ptrA, oldA);
			}
			if (oldD != noReg) {
				*dest << mov(ptrD, oldD);
			}
		}

		void RemoveInvalid::umodTfm(Listing *dest, Instr *instr, Nat line) {
			imodTfm(dest, instr, line);
		}

	}
}
