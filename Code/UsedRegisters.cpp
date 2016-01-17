#include "stdafx.h"
#include "UsedRegisters.h"

namespace code {

	// Check add any read registers from the instructions indirect part.
	void addIndirect(Registers &to, const Value &v) {
		if (v.type() == Value::tRelative) {
			if (v.reg() != ptrStack && v.reg() != ptrFrame)
				to += v.reg();
		}
	}

	void addIndirect(Registers &to, const Instruction &instr) {
		addIndirect(to, instr.src());
		addIndirect(to, instr.dest());
	}

	// Helper for ourselves. Add 'from' if it is a register.
	Registers &operator +=(Registers &to, const Value &from) {
		if (from.type() == Value::tRegister)
			if (from.reg() != ptrStack && from.reg() != ptrFrame)
				to += from.reg();
		return to;
	}

	Registers &operator -=(Registers &to, const Value &from) {
		if (from.type() == Value::tRegister)
			to -= from.reg();
		return to;
	}

	UsedRegisters::UsedRegisters(const Listing &src) : used(src.size()) {
		findRegisters(src);
	}

	void UsedRegisters::findRegisters(const Listing &src) {
		Registers usedNow;

		all = Registers();

		for (nat i = src.size(); i != 0; i--) {
			const Instruction &instr = src[i-1];

			all += processInstr(instr, usedNow);

			used[i-1] = usedNow;
		}

		// Remove unused registers.
		for (nat i = 0; i < src.size(); i++) {
			this->used[i] &= all;
		}
	}

	Registers UsedRegisters::processInstr(const Instruction &instr, Registers &used) {
		Registers write;

		switch (instr.op()) {
		case op::jmp:
		case op::beginBlock:
		case op::endBlock:
		case op::prolog:
			used = Registers();
			break;

		case op::callFloat:
		case op::fnCallFloat:
		case op::fnCall:
		case op::call:
			used = Registers();
			// Intentional fall-thru.

		default:
			addIndirect(used, instr);
			used += instr.src();

			if (instr.destMode() & destWrite) {
				write += instr.dest();
				used -= instr.dest();
			}

			if (instr.destMode() & destRead) {
				used += instr.dest();
			}

			break;
		}

		return write;
	}
}
