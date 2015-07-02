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
			Registers needsPreserve = registers[line];
			add64(needsPreserve);

			Registers used = registers[line];
			for (nat i = 0; i < regs.size(); i++)
				used += regs[i];
			add64(used);

			for (nat i = 0; i < regs.size(); i++) {
				Register r = noReg;
				if (needsPreserve.contains(regs[i]))
					r = preserve(regs[i], used, to);
				result[i] = r;
				if (r != noReg)
					used += r;
			}

			return result;
		}

		void Transform::restore(Register r, Register saved, const Registers &used, Listing &to) const {
			if (!used.contains(r))
				return;

			if (saved == noReg) {
				to << pop(r);
			} else {
				to << mov(r, saved);
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

			assert(size <= Size::sInt, "The 64-bit transform should have fixed this!");

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
			assert(size != Size::sByte && size <= Size::sInt, "Bytes not supported yet");

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

		void idivTfm(const Transform &tfm, Listing &to, nat line) {
			const Instruction &instr = tfm.from[line];
			const Value &dest = instr.dest();

			if (dest.type() == Value::tRegister && asSize(dest.reg(), 0) == ptrA) {
				// Supported!
				to << instr;
				return;
			}

			// The 64-bit transform has been executed before, so we are sure that size is <= sInt
			bool isByte = dest.size() == Size::sByte;

			Registers used = tfm.registers[line];
			add64(used);

			// Move dest into eax first.
			if (used.contains(eax))
				to << push(eax);
			if (!isByte && used.contains(edx))
				to << push(edx);
			to << mov(eax, dest);
			to << instr.alterDest(eax);
			to << mov(dest, eax);

			if (!isByte && used.contains(edx))
				to << pop(edx);
			if (used.contains(eax))
				to << pop(eax);
		}

		void udivTfm(const Transform &tfm, Listing &to, nat line) {
			idivTfm(tfm, to, line);
		}

		void imodTfm(const Transform &tfm, Listing &to, nat line) {
			const Instruction &instr = tfm.from[line];
			const Value &dest = instr.dest();

			bool eaxDest = (dest.type() == Value::tRegister && asSize(dest.reg(), 0) == ptrA);

			// The 64-bit transform has been executed before, so we are sure that size is <= sInt
			bool isByte = dest.size() == Size::sByte;

			Registers used = tfm.registers[line];
			add64(used);

			// Move dest into eax first.
			if (!eaxDest && used.contains(eax))
				to << push(eax);
			if (!isByte && used.contains(edx))
				to << push(edx);
			to << mov(eax, dest);
			if (instr.op() == op::imod)
				to << idiv(eax, instr.src());
			else
				to << udiv(eax, instr.src());
			to << mov(dest, edx);

			if (!isByte && used.contains(edx))
				to << pop(edx);
			if (!eaxDest && used.contains(eax))
				to << pop(eax);
		}

		void umodTfm(const Transform &tfm, Listing &to, nat line) {
			imodTfm(tfm, to, line);
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

		void icastTfm(const Transform &tfm, Listing &to, nat line) {
			const Instruction &instr = tfm.from[line];
			nat sFrom = instr.src().size().current();
			nat sTo = instr.dest().size().current();

			if (instr.dest() == Value(asSize(eax, sTo))) {
				to << instr;
				return;
			}

			Registers used = tfm.registers[line];
			add64(used);
			bool preserveEax = used.contains(eax);
			bool preserveEdx = used.contains(edx);

			if (preserveEax)
				to << push(eax);
			if (preserveEdx)
				to << push(edx);

			if (min(sFrom, sTo) == 1 && max(sFrom, sTo) == 8) {
				to << instr.alterDest(eax);
				to << instr.altered(asSize(eax, sTo), eax);
			} else {
				to << instr.alterDest(asSize(eax, sTo));
			}

			if (sTo == 8) {
				to << mov(low32(instr.dest()), eax);
				to << mov(high32(instr.dest()), edx);
			} else if (sTo <= 4) {
				to << mov(instr.dest(), asSize(eax, sTo));
			}

			if (preserveEdx)
				to << push(edx);
			if (preserveEax)
				to << pop(eax);
		}

		void ucastTfm(const Transform &tfm, Listing &to, nat line) {
			icastTfm(tfm, to, line);
		}

		void addRefTfm(const Transform &tfm, Listing &to, nat line) {
			vector<Register> preserve = regsNotPreserved();
			vector<Register> saved = tfm.preserve(preserve, line, to);

			const Instruction &instr = tfm.from[line];
			to << fnParam(instr.src());
			to << fnCall(Ref(tfm.arena.addRef), retVoid());

			tfm.restore(preserve, saved, line, to);
		}

		void releaseRefTfm(const Transform &tfm, Listing &to, nat line) {
			vector<Register> preserve = regsNotPreserved();
			vector<Register> saved = tfm.preserve(preserve, line, to);

			const Instruction &instr = tfm.from[line];
			to << fnParam(instr.src());
			to << fnCall(Ref(tfm.arena.releaseRef), retVoid());

			tfm.restore(preserve, saved, line, to);
		}

		void retFloatTfm(const Transform &tfm, Listing &to, nat line) {
			const Instruction &instr = tfm.from[line];
			const Size &s = instr.src().size();

			to << push(instr.src());
			to << fld(xRel(s, ptrStack));
			to << add(ptrStack, natPtrConst(s));
			to << ret(retVal(s, false));
		}

		void callFloatTfm(const Transform &tfm, Listing &to, nat line) {
			const Instruction &instr = tfm.from[line];
			const Value &dest = instr.dest();
			const Size &s = dest.size();

			to << call(instr.src(), retVal(s, false));
			to << sub(ptrStack, natPtrConst(s));
			to << fstp(xRel(s, ptrStack));
			to << pop(dest);
		}

		void fnCallFloatTfm(const Transform &tfm, Listing &to, nat line) {
			const Instruction &instr = tfm.from[line];
			const Value &dest = instr.dest();
			const Size &s = dest.size();

			to << fnCall(instr.src(), retVal(s, false));
			to << sub(ptrStack, natPtrConst(s));
			to << fstp(xRel(s, ptrStack));
			to << pop(dest);
		}

	}
}

#endif
