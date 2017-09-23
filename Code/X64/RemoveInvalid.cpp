#include "stdafx.h"
#include "RemoveInvalid.h"
#include "Asm.h"
#include "../Listing.h"
#include "../UsedRegs.h"

namespace code {
	namespace x64 {

#define TRANSFORM(x) { op::x, &RemoveInvalid::x ## Tfm }
#define IMM_REG(x) { op::x, &RemoveInvalid::immRegTfm }

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

			TRANSFORM(lea),
			TRANSFORM(fnCall),
			TRANSFORM(fnParam),
			TRANSFORM(fnParamRef),
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
			assert(reg != noReg, L"We should never run out of registers on X86-64!");
			reg = asSize(reg, size);
			*dest << mov(reg, instr->src());
			*dest << instr->alterSrc(reg);
		}

		void RemoveInvalid::leaTfm(Listing *dest, Instr *instr, Nat line) {
			if (instr->dest().type() == opRegister) {
				*dest << instr;
				return;
			}

			Reg reg = unusedReg(used->at(line));
			assert(reg != noReg, L"We should never run out of registers on X86-64!");
			reg = asSize(reg, Size::sPtr);
			*dest << lea(reg, instr->src());
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

	}
}
