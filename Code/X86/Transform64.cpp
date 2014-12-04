#include "stdafx.h"
#include "Transform64.h"

#ifdef X86
#include "MachineCodeX86.h"
#include "OpTable.h"

namespace code {
	namespace machineX86 {

		// Defines transformed instructions.
#define TFM64(x) { op::x, &Transform64::x ## Tfm }
		typedef void (Transform64::*Tfm64Fn)(Listing &, const Instruction &);

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

			TFM64(push),
			TFM64(pop),
		};


		Transform64::Transform64(const Listing &from) : Transformer(from) {}

		void Transform64::transform(Listing &to, nat line) {
			static OpTable<Tfm64Fn> tfm(transform64Map, ARRAY_SIZE(transform64Map));

			const Instruction &instr = from[line];
			Tfm64Fn f = tfm[instr.op()];

			if (f) {
				(this->*f)(to, instr);
			} else {
				to << instr;
			}
		}


		void Transform64::movTfm(Listing &to, const Instruction &instr) {
			to << code::mov(low32(instr.dest()), low32(instr.src()));
			to << code::mov(high32(instr.dest()), high32(instr.src()));
		}

		void Transform64::addTfm(Listing &to, const Instruction &instr) {
			to << code::add(low32(instr.dest()), low32(instr.src()));
			to << code::adc(high32(instr.dest()), high32(instr.src()));
		}

		void Transform64::adcTfm(Listing &to, const Instruction &instr) {
			to << code::adc(low32(instr.dest()), low32(instr.src()));
			to << code::adc(high32(instr.dest()), high32(instr.src()));
		}

		void Transform64::orTfm(Listing &to, const Instruction &instr) {
			to << code::or(low32(instr.dest()), low32(instr.src()));
			to << code::or(high32(instr.dest()), high32(instr.src()));
		}

		void Transform64::andTfm(Listing &to, const Instruction &instr) {
			to << code::and(low32(instr.dest()), low32(instr.src()));
			to << code::and(high32(instr.dest()), high32(instr.src()));
		}

		void Transform64::subTfm(Listing &to, const Instruction &instr) {
			to << code::sub(low32(instr.dest()), low32(instr.src()));
			to << code::sbb(high32(instr.dest()), high32(instr.src()));
		}

		void Transform64::sbbTfm(Listing &to, const Instruction &instr) {
			to << code::sbb(low32(instr.dest()), low32(instr.src()));
			to << code::sbb(high32(instr.dest()), high32(instr.src()));
		}

		void Transform64::xorTfm(Listing &to, const Instruction &instr) {
			to << code::xor(low32(instr.dest()), low32(instr.src()));
			to << code::xor(high32(instr.dest()), high32(instr.src()));
		}

		void Transform64::cmpTfm(Listing &to, const Instruction &instr) {
			TODO("Implement jne");
			TODO("Test, should work with > < = != at least (maybe not parity)");
			assert(false);
			Label end = to.label();
			to << code::cmp(high32(instr.dest()), high32(instr.src()));
			//to << code::jne(end);
			to << code::cmp(low32(instr.dest()), low32(instr.src()));
			to << end;
		}

		void Transform64::mulTfm(Listing &to, const Instruction &instr) {
			TODO("Implement proper 64-bit multiplication!");
			assert(false);
		}

		void Transform64::pushTfm(Listing &to, const Instruction &instr) {
			to << code::push(high32(instr.src()));
			to << code::push(low32(instr.src()));
		}

		void Transform64::popTfm(Listing &to, const Instruction &instr) {
			to << code::pop(low32(instr.src()));
			to << code::pop(high32(instr.src()));
		}

	}
}

#endif
