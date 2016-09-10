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
			used = code::usedRegisters(src).used;
		}

		void Remove64::during(Listing *dest, Listing *src, Nat line) {
			static OpTable<TransformFn> t(transformMap, ARRAY_COUNT(transformMap));

			Instr *i = src->at(line);
			TransformFn f = t[i->op()];
			if (f) {
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

			*to << push(e, high32(src));
			*to << push(e, low32(src));
			*to << push(e, high32(dest));
			*to << push(e, low32(dest));
			*to << call(e, xConst(Size::sPtr, (Word)fn), ValType(Size::sLong, false));
			*to << add(e, ptrStack, ptrConst(Size::sLong * 2));

			// Save the result somewhere
			if (dest != Operand(rax)) {
				*to << mov(e, high32(dest), high32(rax));
				*to << mov(e, low32(dest), low32(rax));
			}

			// Restore registers
			preserve.restore();
		}

		void Remove64::movTfm(Listing *to, Instr *instr, RegSet *used) {
			Engine &e = engine();

			*to << mov(e, low32(instr->dest()), low32(instr->src()));
			*to << mov(e, high32(instr->dest()), high32(instr->src()));
		}

		void Remove64::addTfm(Listing *to, Instr *instr, RegSet *used) {
			Engine &e = engine();

			*to << add(e, low32(instr->dest()), low32(instr->src()));
			*to << adc(e, high32(instr->dest()), high32(instr->src()));
		}

		void Remove64::adcTfm(Listing *to, Instr *instr, RegSet *used) {
			Engine &e = engine();

			*to << adc(e, low32(instr->dest()), low32(instr->src()));
			*to << adc(e, high32(instr->dest()), high32(instr->src()));
		}

		void Remove64::orTfm(Listing *to, Instr *instr, RegSet *used) {
			Engine &e = engine();

			*to << or(e, low32(instr->dest()), low32(instr->src()));
			*to << or(e, high32(instr->dest()), high32(instr->src()));
		}

		void Remove64::andTfm(Listing *to, Instr *instr, RegSet *used) {
			Engine &e = engine();

			*to << and(e, low32(instr->dest()), low32(instr->src()));
			*to << and(e, high32(instr->dest()), high32(instr->src()));
		}

		void Remove64::subTfm(Listing *to, Instr *instr, RegSet *used) {
			Engine &e = engine();

			*to << sub(e, low32(instr->dest()), low32(instr->src()));
			*to << sbb(e, high32(instr->dest()), high32(instr->src()));
		}

		void Remove64::sbbTfm(Listing *to, Instr *instr, RegSet *used) {
			Engine &e = engine();

			*to << sbb(e, low32(instr->dest()), low32(instr->src()));
			*to << sbb(e, high32(instr->dest()), high32(instr->src()));
		}

		void Remove64::xorTfm(Listing *to, Instr *instr, RegSet *used) {
			Engine &e = engine();

			*to << xor(e, low32(instr->dest()), low32(instr->src()));
			*to << xor(e, high32(instr->dest()), high32(instr->src()));
		}

		void Remove64::cmpTfm(Listing *to, Instr *instr, RegSet *used) {
			Engine &e = engine();

			*to << mov(e, low32(instr->dest()), low32(instr->src()));
			*to << mov(e, high32(instr->dest()), high32(instr->src()));
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
			Engine &e = engine();

			*to << push(e, high32(instr->src()));
			*to << push(e, low32(instr->src()));
		}

		void Remove64::popTfm(Listing *to, Instr *instr, RegSet *used) {
			Engine &e = engine();

			*to << pop(e, low32(instr->dest()));
			*to << pop(e, high32(instr->dest()));
		}

	}
}
