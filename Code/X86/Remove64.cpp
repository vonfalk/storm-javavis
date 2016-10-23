#include "stdafx.h"
#include "Remove64.h"
#include "Asm.h"

namespace code {
	namespace x86 {

#define TRANSFORM(x) { op::x, &Remove64::x ## Tfm }

		const OpEntry<Remove64::TransformFn> Remove64::transformMap[] = {
			TRANSFORM(mov),
			TRANSFORM(add),
			TRANSFORM(adc),
			TRANSFORM(or),
			TRANSFORM(and),
			TRANSFORM(sub),
			TRANSFORM(sbb),
			TRANSFORM(xor),
			TRANSFORM(cmp),
			TRANSFORM(mul),
			TRANSFORM(idiv),
			TRANSFORM(udiv),
			TRANSFORM(imod),
			TRANSFORM(umod),
			TRANSFORM(push),
			TRANSFORM(pop),
		};

		Remove64::Remove64() {}

		void Remove64::before(Listing *dest, Listing *src) {
			used = code::usedRegs(dest->arena, src).used;
		}

		void Remove64::during(Listing *dest, Listing *src, Nat line) {
			static OpTable<TransformFn> t(transformMap, ARRAY_COUNT(transformMap));

			Instr *i = src->at(line);
			TransformFn f = t[i->op()];
			if (i->size() == Size::sLong && f != null) {
				(this->*f)(dest, i, used->at(line));
			} else {
				*dest << i;
			}
		}

		// Call a function which implements the instruction. (reads src and dest, stores in dest).
		static void callFn(Listing *to, Instr *instr, RegSet *used, const void *fn) {
			Engine &e = to->engine();
			Operand src = instr->src();
			Operand dest = instr->dest();

			used = new (used) RegSet(used);

			// If we're supposed to clobber a register, don't try to preserve it!
			if (dest.type() == opRegister)
				used->remove(dest.reg());

			// Save registers
			Preserve preserve(fnDirtyRegs(e), used, to);

			*to << push(high32(src));
			*to << push(low32(src));
			*to << push(high32(dest));
			*to << push(low32(dest));
			*to << call(xConst(Size::sPtr, (Word)fn), ValType(Size::sLong, false));
			*to << add(ptrStack, ptrConst(Size::sLong * 2));

			// Save the result somewhere
			if (dest != Operand(rax)) {
				*to << mov(high32(dest), high32(rax));
				*to << mov(low32(dest), low32(rax));
			}

			// Restore registers
			preserve.restore();
		}

		void Remove64::movTfm(Listing *to, Instr *instr, RegSet *used) {
			*to << mov(low32(instr->dest()), low32(instr->src()));
			*to << mov(high32(instr->dest()), high32(instr->src()));
		}

		void Remove64::addTfm(Listing *to, Instr *instr, RegSet *used) {
			*to << add(low32(instr->dest()), low32(instr->src()));
			*to << adc(high32(instr->dest()), high32(instr->src()));
		}

		void Remove64::adcTfm(Listing *to, Instr *instr, RegSet *used) {
			*to << adc(low32(instr->dest()), low32(instr->src()));
			*to << adc(high32(instr->dest()), high32(instr->src()));
		}

		void Remove64::orTfm(Listing *to, Instr *instr, RegSet *used) {
			*to << or(low32(instr->dest()), low32(instr->src()));
			*to << or(high32(instr->dest()), high32(instr->src()));
		}

		void Remove64::andTfm(Listing *to, Instr *instr, RegSet *used) {
			*to << and(low32(instr->dest()), low32(instr->src()));
			*to << and(high32(instr->dest()), high32(instr->src()));
		}

		void Remove64::subTfm(Listing *to, Instr *instr, RegSet *used) {
			*to << sub(low32(instr->dest()), low32(instr->src()));
			*to << sbb(high32(instr->dest()), high32(instr->src()));
		}

		void Remove64::sbbTfm(Listing *to, Instr *instr, RegSet *used) {
			*to << sbb(low32(instr->dest()), low32(instr->src()));
			*to << sbb(high32(instr->dest()), high32(instr->src()));
		}

		void Remove64::xorTfm(Listing *to, Instr *instr, RegSet *used) {
			*to << xor(low32(instr->dest()), low32(instr->src()));
			*to << xor(high32(instr->dest()), high32(instr->src()));
		}

		void Remove64::cmpTfm(Listing *to, Instr *instr, RegSet *used) {
			Reg dest = unusedReg(used);

			bool preserved = false;
			if (dest == noReg) {
				*to << push(ebx);
				dest = ebx;
				preserved = true;
			} else {
				dest = asSize(dest, Size::sInt);
			}

			*to << mov(dest, low32(instr->dest()));
			*to << sub(dest, low32(instr->src()));
			*to << pushFlags();
			*to << mov(dest, high32(instr->dest()));
			*to << sbb(dest, high32(instr->src()));
			*to << pushFlags();

			// Reset ZF if it was not set the first time around.
			*to << mov(dest, intRel(ptrStack, Offset::sInt));
			*to << or(dest, intConst(~(1 << 6))); // Now all bits except for ZF are always 1
			*to << and(intRel(ptrStack, Offset()), dest); // Masking out ZF.

			*to << popFlags();
			*to << pop(dest); // We can not use add here, as it modifies flags.

			if (preserved) {
				*to << pop(ebx);
			}

			// Wrong:
			// Label end = to->label();
			// *to << cmp(high32(instr->dest()), high32(instr->src()));
			// *to << jmp(end, ifNotEqual);
			// *to << cmp(low32(instr->dest()), low32(instr->src()));
			// *to << end;
		}

		static Long CODECALL mul(Long a, Long b) {
			return a * b;
		}

		void Remove64::mulTfm(Listing *to, Instr *instr, RegSet *used) {
			callFn(to, instr, used, &mul);
		}

		static Long CODECALL idiv(Long a, Long b) {
			return a / b;
		}

		void Remove64::idivTfm(Listing *to, Instr *instr, RegSet *used) {
			callFn(to, instr, used, &idiv);
		}

		static Word CODECALL udiv(Word a, Word b) {
			return a / b;
		}

		void Remove64::udivTfm(Listing *to, Instr *instr, RegSet *used) {
			callFn(to, instr, used, &udiv);
		}

		static Long CODECALL imod(Long a, Long b) {
			return a % b;
		}

		void Remove64::imodTfm(Listing *to, Instr *instr, RegSet *used) {
			callFn(to, instr, used, &imod);
		}

		static Word CODECALL umod(Word a, Word b) {
			return a % b;
		}

		void Remove64::umodTfm(Listing *to, Instr *instr, RegSet *used) {
			callFn(to, instr, used, &umod);
		}

		void Remove64::pushTfm(Listing *to, Instr *instr, RegSet *used) {
			*to << push(high32(instr->src()));
			*to << push(low32(instr->src()));
		}

		void Remove64::popTfm(Listing *to, Instr *instr, RegSet *used) {
			*to << pop(low32(instr->dest()));
			*to << pop(high32(instr->dest()));
		}

	}
}
