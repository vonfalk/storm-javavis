#include "stdafx.h"
#include "Transform64.h"

#ifdef X86
#include "MachineCodeX86.h"
#include "OpTable.h"

namespace code {
	namespace machineX86 {

		// Defines transformed instructions.
#define TFM64(x) { op::x, &Transform64::x ## Tfm }
		typedef void (Transform64::*Tfm64Fn)(Listing &, const Instruction &, const Registers &);

		static OpEntry<Tfm64Fn> transform64Map[] = {
			TFM64(mov),
			TFM64(add),
			TFM64(adc),
			TFM64(or),
			TFM64(and),
			TFM64(sub),
			TFM64(sbb),
			TFM64(xor),
			TFM64(cmp),
			TFM64(mul),
			TFM64(idiv),
			TFM64(udiv),
			TFM64(imod),
			TFM64(umod),

			TFM64(fnParam),
			TFM64(push),
			TFM64(pop),
		};


		Transform64::Transform64(const Listing &from) : Transformer(from), registers(from) {}

		void Transform64::transform(Listing &to, nat line) {
			static OpTable<Tfm64Fn> tfm(transform64Map, ARRAY_SIZE(transform64Map));

			const Instruction &instr = from[line];
			Tfm64Fn f = tfm[instr.op()];

			if (f && instr.currentSize() == 8) {
				(this->*f)(to, instr, registers[line]);
			} else {
				to << instr;
			}
		}

		// Push any used registers that are not preserved during function calls onto the stack.
		static void pushUsed(Listing &to, Registers used) {
			add64(used);
			vector<Register> r = regsNotPreserved();
			for (nat i = 0; i < r.size(); i++) {
				if (used.contains(r[i]))
					to << push(r[i]);
			}
		}

		static void popUsed(Listing &to, Registers used) {
			add64(used);
			vector<Register> r = regsNotPreserved();
			for (nat i = r.size(); i > 0; i--) {
				if (used.contains(r[i - 1]))
					to << pop(r[i - 1]);
			}
		}

		static bool isPreserved(Register reg) {
			vector<Register> r = regsNotPreserved();
			for (nat i = 0; i < r.size(); i++)
				if (asSize(r[i], 0) == asSize(reg, 0))
					return false;
			return true;
		}

		// Call a function instead.
		static void callFn(Listing &out, const Instruction &instr, const Registers &used, void *fn) {
			Value to = instr.dest();
			Register destOriginal = noReg;
			if (to.type() == Value::tRelative && !isPreserved(to.reg())) {
				// We need to preserve this register. Save it!
				destOriginal = to.reg();
				// We will use ptrC later, that is always safe to do!
				to = longRel(ptrC, to.offset());
			}

			pushUsed(out, used);
			if (destOriginal != noReg) {
				// Need to preserve ptrC?
				if (used.contains(ptrC))
					out << code::push(ptrC);
				// Preserve our target register.
				out << code::push(destOriginal);
			}
			out << code::push(high32(instr.src()));
			out << code::push(low32(instr.src()));
			out << code::push(high32(instr.dest()));
			out << code::push(low32(instr.dest()));
			out << code::call(ptrConst(fn), Size::sLong);
			out << code::add(ptrStack, intPtrConst(4 * 4));

			// Restore our target if needed.
			if (destOriginal != noReg)
				out << code::pop(ptrC);
			out << code::mov(high32(to), high32(rax));
			out << code::mov(low32(to), low32(rax));

			// Restore whatever was in ptrC before (if needed).
			if (destOriginal != noReg && used.contains(ptrC))
				out << code::pop(ptrC);
			popUsed(out, used);
		}

		void Transform64::movTfm(Listing &to, const Instruction &instr, const Registers &used) {
			to << code::mov(low32(instr.dest()), low32(instr.src()));
			to << code::mov(high32(instr.dest()), high32(instr.src()));
		}

		void Transform64::addTfm(Listing &to, const Instruction &instr, const Registers &used) {
			to << code::add(low32(instr.dest()), low32(instr.src()));
			to << code::adc(high32(instr.dest()), high32(instr.src()));
		}

		void Transform64::adcTfm(Listing &to, const Instruction &instr, const Registers &used) {
			to << code::adc(low32(instr.dest()), low32(instr.src()));
			to << code::adc(high32(instr.dest()), high32(instr.src()));
		}

		void Transform64::orTfm(Listing &to, const Instruction &instr, const Registers &used) {
			to << code::or(low32(instr.dest()), low32(instr.src()));
			to << code::or(high32(instr.dest()), high32(instr.src()));
		}

		void Transform64::andTfm(Listing &to, const Instruction &instr, const Registers &used) {
			to << code::and(low32(instr.dest()), low32(instr.src()));
			to << code::and(high32(instr.dest()), high32(instr.src()));
		}

		void Transform64::subTfm(Listing &to, const Instruction &instr, const Registers &used) {
			to << code::sub(low32(instr.dest()), low32(instr.src()));
			to << code::sbb(high32(instr.dest()), high32(instr.src()));
		}

		void Transform64::sbbTfm(Listing &to, const Instruction &instr, const Registers &used) {
			to << code::sbb(low32(instr.dest()), low32(instr.src()));
			to << code::sbb(high32(instr.dest()), high32(instr.src()));
		}

		void Transform64::xorTfm(Listing &to, const Instruction &instr, const Registers &used) {
			to << code::xor(low32(instr.dest()), low32(instr.src()));
			to << code::xor(high32(instr.dest()), high32(instr.src()));
		}

		void Transform64::cmpTfm(Listing &to, const Instruction &instr, const Registers &used) {
			Label end = to.label();
			to << code::cmp(high32(instr.dest()), high32(instr.src()));
			to << code::jmp(end, ifNotEqual);
			to << code::cmp(low32(instr.dest()), low32(instr.src()));
			to << end;
		}

		static Long CODECALL mul(Long a, Long b) {
			return a * b;
		}

		void Transform64::mulTfm(Listing &to, const Instruction &instr, const Registers &used) {
			callFn(to, instr, used, &mul);
		}

		static Long CODECALL idiv(Long a, Long b) {
			return a / b;
		}

		void Transform64::idivTfm(Listing &to, const Instruction &instr, const Registers &used) {
			callFn(to, instr, used, &idiv);
		}

		static Word CODECALL udiv(Word a, Word b) {
			return a / b;
		}

		void Transform64::udivTfm(Listing &to, const Instruction &instr, const Registers &used) {
			callFn(to, instr, used, &udiv);
		}

		static Long CODECALL imod(Long a, Long b) {
			return a % b;
		}

		void Transform64::imodTfm(Listing &to, const Instruction &instr, const Registers &used) {
			callFn(to, instr, used, &imod);
		}

		static Word CODECALL umod(Word a, Word b) {
			return a % b;
		}

		void Transform64::umodTfm(Listing &to, const Instruction &instr, const Registers &used) {
			callFn(to, instr, used, &umod);
		}

		void Transform64::pushTfm(Listing &to, const Instruction &instr, const Registers &used) {
			to << code::push(high32(instr.src()));
			to << code::push(low32(instr.src()));
		}

		void Transform64::popTfm(Listing &to, const Instruction &instr, const Registers &used) {
			to << code::pop(low32(instr.src()));
			to << code::pop(high32(instr.src()));
		}

		void Transform64::fnParamTfm(Listing &to, const Instruction &instr, const Registers &used) {
			const Value &param = instr.src();
			const Value &copy = instr.dest();
			if (copy == Value()) {
				to << code::fnParam(low32(param));
				to << code::fnParam(high32(param));
			} else {
				to << instr;
			}
		}

	}
}

#endif
