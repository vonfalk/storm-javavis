#include "stdafx.h"
#include "RemoveInvalid.h"
#include "Listing.h"

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
			TODO(L"Prepare information about used registers here!");
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

		void RemoveInvalid::immRegTfm(Listing *dest, Listing *src, Nat id) {
			Instr *instr = src->at(id);

			if (supported(instr)) {
				*dest << instr;
				return;
			}

			TODO(L"Fix this!");
		}

		void RemoveInvalid::leaTfm(Listing *dest, Listing *src, Nat id) {}

		void RemoveInvalid::mulTfm(Listing *dest, Listing *src, Nat id) {}

		void RemoveInvalid::idivTfm(Listing *dest, Listing *src, Nat id) {}

		void RemoveInvalid::udivTfm(Listing *dest, Listing *src, Nat id) {}

		void RemoveInvalid::imodTfm(Listing *dest, Listing *src, Nat id) {}

		void RemoveInvalid::umodTfm(Listing *dest, Listing *src, Nat id) {}

		void RemoveInvalid::setCondTfm(Listing *dest, Listing *src, Nat id) {}

		void RemoveInvalid::shlTfm(Listing *dest, Listing *src, Nat id) {}

		void RemoveInvalid::shrTfm(Listing *dest, Listing *src, Nat id) {}

		void RemoveInvalid::sarTfm(Listing *dest, Listing *src, Nat id) {}

		void RemoveInvalid::icastTfm(Listing *dest, Listing *src, Nat id) {}

		void RemoveInvalid::ucastTfm(Listing *dest, Listing *src, Nat id) {}

		void RemoveInvalid::callFloatTfm(Listing *dest, Listing *src, Nat id) {}

		void RemoveInvalid::retFloatTfm(Listing *dest, Listing *src, Nat id) {}

		void RemoveInvalid::fnCallFloatTfm(Listing *dest, Listing *src, Nat id) {}

	}
}
