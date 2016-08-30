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
		};

		RemoveInvalid::RemoveInvalid() {}

		void RemoveInvalid::before(Listing *dest, Listing *src) {
			used = usedRegisters(src).used;
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

		void RemoveInvalid::mulTfm(Listing *dest, Listing *src, Nat line) {}

		void RemoveInvalid::idivTfm(Listing *dest, Listing *src, Nat line) {}

		void RemoveInvalid::udivTfm(Listing *dest, Listing *src, Nat line) {}

		void RemoveInvalid::imodTfm(Listing *dest, Listing *src, Nat line) {}

		void RemoveInvalid::umodTfm(Listing *dest, Listing *src, Nat line) {}

		void RemoveInvalid::setCondTfm(Listing *dest, Listing *src, Nat line) {}

		void RemoveInvalid::shlTfm(Listing *dest, Listing *src, Nat line) {}

		void RemoveInvalid::shrTfm(Listing *dest, Listing *src, Nat line) {}

		void RemoveInvalid::sarTfm(Listing *dest, Listing *src, Nat line) {}

		void RemoveInvalid::icastTfm(Listing *dest, Listing *src, Nat line) {}

		void RemoveInvalid::ucastTfm(Listing *dest, Listing *src, Nat line) {}

		void RemoveInvalid::callFloatTfm(Listing *dest, Listing *src, Nat line) {}

		void RemoveInvalid::retFloatTfm(Listing *dest, Listing *src, Nat line) {}

		void RemoveInvalid::fnCallFloatTfm(Listing *dest, Listing *src, Nat line) {}

	}
}
