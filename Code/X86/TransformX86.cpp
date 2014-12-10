#include "stdafx.h"
#include "TransformX86.h"

#ifdef X86
#include "MachineCodeX86.h"

namespace code {
	namespace machineX86 {

		Transform::Transform(const Listing &from) : Transformer(from), registers(from) {}

		void Transform::transform(Listing &to, nat line) {
			const Instruction &instr = from[line];

			if (TransformFn fn = transformFn(instr.op())) {
				(*fn)(*this, to, line);
			} else {
				to << instr;
			}
		}


		Register Transform::unusedReg(nat line) const {
			Registers regs = registers[line];
			add64(regs);

			Register order[] = { ptrD, ptrSi, ptrDi };

			for (nat i = 0; i < ARRAY_SIZE(order); i++) {
				if (!regs.contains(order[i]))
					return order[i];
			}

			return noReg;
		}


		/**
		 * ImmReg combination already supported?
		 */
		static bool supported(const Instruction &instr) {
			switch (instr.src().type()) {
			case Value::tLabel:
			case Value::tReference:
			case Value::tConstant:
			case Value::tRegister:
				return true;
			default:
				if (instr.dest().type() == Value::tRegister) {
					return true;
				}
				break;
			}

			return false;
		}


		// Split immreg instructions into two if needed.
		void immRegTfm(const Transform &tfm, Listing &to, nat line) {
			const Instruction &instr = tfm.from[line];

			if (supported(instr)) {
				to << instr;
				return;
			}

			nat size = instr.src().size();

			assert(("The 64-bit transform should have fixed this!", size <= 4));

			Register reg = tfm.unusedReg(line);

			if (reg == noReg) {
				reg = asSize(ptrD, size);
				to << code::push(ptrD);
				to << code::mov(reg, instr.src());
				to << instr.alterSrc(reg);
				to << code::pop(ptrD);
			} else {
				reg = asSize(reg, size);
				to << code::mov(reg, instr.src());
				to << instr.alterSrc(reg);
			}
		}

		void mulTfm(const Transform &tfm, Listing &to, nat line) {
			const Instruction &instr = tfm.from[line];

			nat size = instr.size();
			assert(("Bytes not supported yet", size != 1 && size <= 4));

			if (instr.src().type() == Value::tRegister) {
				to << instr;
				return;
			}

			// Only supported mode is imul <reg>, <r/m>, move src into a register.
			Register reg = tfm.unusedReg(line);
			if (reg == noReg) {
				reg = asSize(ptrD, size);
				to << code::push(ptrD);
				to << code::mov(reg, instr.dest());
				to << instr.alterDest(reg);
				to << code::mov(instr.dest(), reg);
				to << code::pop(ptrD);
			} else {
				reg = asSize(reg, size);
				to << code::mov(reg, instr.dest());
				to << instr.alterDest(reg);
				to << code::mov(instr.dest(), reg);
			}
		}

		void setCondTfm(const Transform &tfm, Listing &to, nat line) {
			const Instruction &instr = tfm.from[line];
			switch (instr.src().constant()) {
			case ifAlways:
				to << mov(instr.dest(), byteConst(1));
				break;
			case ifNever:
				to << mov(instr.dest(), byteConst(0));
				break;
			default:
				to << instr;
				break;
			}
		}

	}
}

#endif
