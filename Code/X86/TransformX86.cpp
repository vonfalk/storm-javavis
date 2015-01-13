#include "stdafx.h"
#include "TransformX86.h"

#ifdef X86
#include "MachineCodeX86.h"

namespace code {
	namespace machineX86 {

		Transform::Transform(const Listing &from, Arena &arena)
			: Transformer(from), arena(arena), registers(from) {}

		void Transform::transform(Listing &to, nat line) {
			const Instruction &instr = from[line];

			if (TransformFn fn = transformFn(instr.op())) {
				(*fn)(*this, to, line);
			} else {
				to << instr;
			}
		}

		Register Transform::unusedReg(const Registers &regs) const {
			Register order[] = { ptrD, ptrSi, ptrDi };

			for (nat i = 0; i < ARRAY_SIZE(order); i++) {
				if (!regs.contains(order[i]))
					return order[i];
			}

			return noReg;
		}

		Register Transform::unusedReg(nat line) const {
			Registers regs = registers[line];
			add64(regs);

			return unusedReg(regs);
		}

		Register Transform::preserve(Register r, const Registers &used, Listing &to) const {
			if (!used.contains(r))
				return noReg;

			Register into = unusedReg(used);
			if (into == noReg) {
				to << push(r);
			} else {
				into = asSize(into, size(r));
				to << mov(into, r);
			}
			return into;
		}

		vector<Register> Transform::preserve(const vector<Register> &regs, nat line, Listing &to) const {
			vector<Register> result(regs.size(), noReg);
			Registers used = registers[line];
			add64(used);

			for (nat i = 0; i < regs.size(); i++) {
				Register r = preserve(regs[i], used, to);
				if (r != noReg)
					used += r;
				result[i] = r;
			}

			return result;
		}

		void Transform::restore(Register r, Register saved, const Registers &used, Listing &to) const {
			if (!used.contains(r))
				return;

			if (saved == noReg) {
				to << pop(r);
			} else {
				to << mov(saved, r);
			}
		}

		void Transform::restore(const vector<Register> &regs, const vector<Register> &saved, nat line, Listing &to) const {
			Registers used = registers[line];
			add64(used);

			for (nat i = regs.size(); i > 0; i--)
				restore(regs[i-1], saved[i-1], used, to);
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

			Size size = instr.src().size();

			assert(("The 64-bit transform should have fixed this!", size <= Size::sInt));

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

		void leaTfm(const Transform &tfm, Listing &to, nat line) {
			const Instruction &instr = tfm.from[line];
			if (instr.dest().type() == Value::tRegister) {
				to << instr;
				return;
			}

			Register reg = tfm.unusedReg(line);

			if (reg == noReg) {
				to << code::push(ptrD);
				to << code::lea(ptrD, instr.src());
				to << code::mov(instr.dest(), ptrD);
				to << code::pop(ptrD);
			} else {
				reg = asSize(reg, 0);
				to << code::lea(reg, instr.src());
				to << code::mov(instr.dest(), reg);
			}
		}

		void mulTfm(const Transform &tfm, Listing &to, nat line) {
			const Instruction &instr = tfm.from[line];

			Size size = instr.size();
			assert(("Bytes not supported yet", size != Size::sByte && size <= Size::sInt));

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
			switch (instr.src().condFlag()) {
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

		void shlTfm(const Transform &tfm, Listing &to, nat line) {
			const Instruction &instr = tfm.from[line];

			switch (instr.src().type()) {
			case Value::tRegister:
				if (instr.src().reg() != cl)
					break;
				// Fall-thru
			case Value::tConstant:
				// Supported directly!
				to << instr;
				return;
			}

			// We need to store the value in cl.
			Register reg = tfm.unusedReg(line);
			if (reg != noReg)
				reg = asSize(reg, 4);

			if (instr.dest().type() == Value::tRegister && asSize(instr.dest().reg(), 4) == ecx) {
				Size size = instr.dest().size();
				reg = asSize(reg, size);

				if (reg == noReg) {
					// No free registers.
					to << push(ecx);
					to << mov(cl, instr.src());
					to << instr.alterDest(xRel(size, ptrStack, Offset(0))).alterSrc(cl);
					to << pop(ecx);
				} else {
					to << mov(reg, instr.dest());
					to << mov(cl, instr.src());
					to << instr.alterDest(reg).alterSrc(cl);
					to << mov(instr.dest(), reg);
				}

				return;
			}

			if (reg == noReg)
				to << push(ecx);
			else
				to << mov(reg, ecx);

			to << mov(cl, instr.src());
			to << instr.alterSrc(cl);

			if (reg == noReg)
				to << pop(ecx);
			else
				to << mov(ecx, reg);
		}

		void shrTfm(const Transform &tfm, Listing &to, nat line) {
			shlTfm(tfm, to, line);
		}

		void sarTfm(const Transform &tfm, Listing &to, nat line) {
			shlTfm(tfm, to, line);
		}

		void addRefTfm(const Transform &tfm, Listing &to, nat line) {
			vector<Register> preserve = regsNotPreserved();
			vector<Register> saved = tfm.preserve(preserve, line, to);

			const Instruction &instr = tfm.from[line];
			to << fnParam(instr.src());
			to << fnCall(Ref(tfm.arena.addRef), Size());

			tfm.restore(preserve, saved, line, to);
		}

		void releaseRefTfm(const Transform &tfm, Listing &to, nat line) {
			vector<Register> preserve = regsNotPreserved();
			vector<Register> saved = tfm.preserve(preserve, line, to);

			const Instruction &instr = tfm.from[line];
			to << fnParam(instr.src());
			to << fnCall(Ref(tfm.arena.releaseRef), Size());

			tfm.restore(preserve, saved, line, to);
		}

	}
}

#endif
