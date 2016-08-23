#include "stdafx.h"
#include "Instr.h"
#include "Exception.h"
#include "Core/StrBuf.h"

namespace code {

	Instr::Instr() : iOp(op::nop) {}

	Instr::Instr(const Instr &o) : iOp(o.iOp), iSrc(o.iSrc), iDest(o.iDest) {}

	Instr::Instr(op::Code op, const Operand &dest, const Operand &src) : iOp(op), iDest(dest), iSrc(src) {}

	void Instr::deepCopy(CloneEnv *env) {
		// Everything is constant here anyway. No need!
	}

	op::Code Instr::op() const {
		return iOp;
	}

	DestMode Instr::mode() const {
		return destMode(iOp);
	}

	Operand Instr::src() const {
		return iSrc;
	}

	Operand Instr::dest() const {
		return iDest;
	}

	Instr *Instr::alter(Operand dest, Operand src) {
		return new (this) Instr(iOp, dest, src);
	}

	Instr *Instr::alterSrc(Operand src) {
		return new (this) Instr(iOp, iDest, src);
	}

	Instr *Instr::alterDest(Operand dest) {
		return new (this) Instr(iOp, dest, iSrc);
	}

	void Instr::toS(StrBuf *to) const {
		*to << code::name(iOp);

		if (iSrc.empty() && iDest.empty()) {
			// No operands.
		} else if (iDest.empty()) {
			*to << L" " << iSrc;
		} else if (iSrc.empty()) {
			*to << L" " << iDest;
		} else {
			*to << L" " << iDest << L", " << iSrc;
		}
	}

	Instr *instr(EnginePtr e, op::Code op) {
		return new (e.v) Instr(op, Operand(), Operand());
	}

	Instr *instrSrc(EnginePtr e, op::Code op, Operand src) {
		src.ensureReadable(op);
		return new (e.v) Instr(op, Operand(), src);
	}

	Instr *instrDest(EnginePtr e, op::Code op, Operand dest) {
		DestMode mode = destMode(op);
		if (mode == destNone)
			throw InvalidValue(L"Can not pass 'destNone' to 'instrDest'.");
		if (mode & destRead)
			dest.ensureReadable(op);
		if (mode & destWrite)
			dest.ensureWritable(op);
		return new (e.v) Instr(op, dest, Operand());
	}

	Instr *instrDestSrc(EnginePtr e, op::Code op, Operand dest, Operand src) {
		DestMode mode = destMode(op);
		if (mode == destNone)
			throw InvalidValue(L"Can not pass 'destNone' to 'instrDestSrc'.");
		if (mode & destRead)
			dest.ensureReadable(op);
		if (mode & destWrite)
			dest.ensureWritable(op);
		src.ensureReadable(op);
		if (dest.size() != src.size())
			throw InvalidValue(L"For " + ::toS(name(op)) + L": Size of operands must match! " +
							::toS(dest) + L" vs " + ::toS(src));
		return new (e.v) Instr(op, dest, src);
	}

	Instr *instrLoose(EnginePtr e, op::Code op, Operand dest, Operand src) {
		return new (e.v) Instr(op, dest, src);
	}

}
