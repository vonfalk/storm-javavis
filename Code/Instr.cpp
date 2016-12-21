#include "stdafx.h"
#include "Instr.h"
#include "Exception.h"
#include "Reg.h"
#include "Core/StrBuf.h"

namespace code {

	Instr::Instr() : iOp(op::nop) {}

	Instr::Instr(const Instr &o) : iOp(o.iOp), iSrc(o.iSrc), iDest(o.iDest) {}

	Instr::Instr(op::Code op, const Operand &dest, const Operand &src) : iOp(op), iDest(dest), iSrc(src) {}

	void Instr::deepCopy(CloneEnv *env) {
		// Everything is constant here anyway. No need!
	}

	Size Instr::size() const {
		if (iSrc.size() > iDest.size())
			return iSrc.size();
		else
			return iDest.size();
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

	/**
	 * Instructions.
	 */

	static Operand sizedReg(Reg base, Size size) {
		Reg s = asSize(base, size);
		if (s == noReg) {
			if (size == Size())
				return Operand();
			else
				throw InvalidValue(L"The return size must fit in a register (ie. < 8 bytes).");
		} else {
			return Operand(s);
		}
	}

	Instr *mov(EnginePtr e, Operand to, Operand from) {
		return instrDestSrc(e, op::mov, to, from);
	}

	Instr *lea(EnginePtr e, Operand to, Operand from) {
		if (to.size() != Size::sPtr)
			throw InvalidValue(L"Lea must update a pointer.");

		switch (from.type()) {
		case opRelative:
		case opVariable:
		case opReference:
			// These are ok.
			break;
		default:
			throw InvalidValue(L"lea must be used with a complex addressing mode or a reference."
							L" (Got " + toS(from) + L")");
		}
		return instrLoose(e, op::lea, to, from);
	}

	Instr *push(EnginePtr e, Operand v) {
		return instrSrc(e, op::push, v);
	}

	Instr *pop(EnginePtr e, Operand to) {
		return instrDest(e, op::pop, to);
	}

	Instr *pushFlags(EnginePtr e) {
		return instr(e, op::pushFlags);
	}

	Instr *popFlags(EnginePtr e) {
		return instr(e, op::popFlags);
	}

	Instr *jmp(EnginePtr e, Operand to) {
		return jmp(e, to, ifAlways);
	}

	Instr *jmp(EnginePtr e, Operand to, CondFlag cond) {
		if (to.size() != Size::sPtr)
			throw InvalidValue(L"Must jump to a pointer, trying to jump to " + toS(to));
		return instrLoose(e, op::jmp, to, cond);
	}

	Instr *setCond(EnginePtr e, Operand to, CondFlag cond) {
		if (to.size() != Size::sByte)
			throw InvalidValue(L"Must set a byte.");
		return instrLoose(e, op::setCond, to, cond);
	}

	Instr *call(EnginePtr e, Operand to, ValType ret) {
		if (to.size() != Size::sPtr)
			throw InvalidValue(L"Must call a pointer, tried calling " + ::toS(to));

		op::Code op = ret.isFloat ? op::callFloat : op::call;
		return instrLoose(e, op, sizedReg(ptrA, ret.size), to);
	}

	Instr *ret(EnginePtr e, ValType ret) {
		Operand r = sizedReg(ptrA, ret.size);
		op::Code op = ret.isFloat ? op::retFloat : op::ret;
		if (r.type() == opNone)
			return instr(e, op);
		else
			return instrSrc(e, op, r);
	}

	Instr *fnParam(EnginePtr e, Operand src) {
		return instrSrc(e, op::fnParam, src);
	}

	Instr *fnParam(EnginePtr e, Var src, Operand copyFn) {
		if (copyFn.type() != opNone) {
			if (copyFn.type() == opConstant)
				throw InvalidValue(L"Should not call constant values, use references instead!");
			if (copyFn.size() != Size::sPtr)
				throw InvalidValue(L"Must call a pointer, tried calling " + ::toS(copyFn));
		}
		return instrLoose(e, op::fnParam, copyFn, src);
	}

	Instr *fnParamRef(EnginePtr e, Operand src, Size size) {
		return instrSrc(e, op::fnParamRef, src.referTo(size));
	}

	Instr *fnParamRef(EnginePtr e, Operand src, Size size, Operand copyFn) {
		if (copyFn.type() != opNone) {
			if (copyFn.type() == opConstant)
				throw InvalidValue(L"Should not call constant values, use references instead!");
			if (copyFn.size() != Size::sPtr)
				throw InvalidValue(L"Must call a pointer, tried calling " + ::toS(copyFn));
		}
		return instrLoose(e, op::fnParamRef, copyFn, src.referTo(size));
	}

	Instr *fnCall(EnginePtr e, Operand src, ValType ret) {
		if (src.type() == opConstant)
			throw InvalidValue(L"Should not call constant values, use references instead!");
		if (src.size() != Size::sPtr)
			throw InvalidValue(L"Must call a pointer, tried calling " + ::toS(src));

		op::Code op = ret.isFloat ? op::fnCallFloat : op::fnCall;
		return instrLoose(e, op, sizedReg(ptrA, ret.size), src);
	}

	Instr *or(EnginePtr e, Operand dest, Operand src) {
		return instrDestSrc(e, op::or, dest, src);
	}

	Instr *and(EnginePtr e, Operand dest, Operand src) {
		return instrDestSrc(e, op::and, dest, src);
	}

	Instr *xor(EnginePtr e, Operand dest, Operand src) {
		return instrDestSrc(e, op::xor, dest, src);
	}

	Instr *not(EnginePtr e, Operand dest) {
		return instrDest(e, op::not, dest);
	}

	Instr *add(EnginePtr e, Operand dest, Operand src) {
		return instrDestSrc(e, op::add, dest, src);
	}

	Instr *adc(EnginePtr e, Operand dest, Operand src) {
		return instrDestSrc(e, op::adc, dest, src);
	}

	Instr *sub(EnginePtr e, Operand dest, Operand src) {
		return instrDestSrc(e, op::sub, dest, src);
	}

	Instr *sbb(EnginePtr e, Operand dest, Operand src) {
		return instrDestSrc(e, op::sbb, dest, src);
	}

	Instr *cmp(EnginePtr e, Operand dest, Operand src) {
		return instrDestSrc(e, op::cmp, dest, src);
	}

	Instr *mul(EnginePtr e, Operand dest, Operand src) {
		return instrDestSrc(e, op::mul, dest, src);
	}

	Instr *idiv(EnginePtr e, Operand dest, Operand src) {
		return instrDestSrc(e, op::idiv, dest, src);
	}

	Instr *udiv(EnginePtr e, Operand dest, Operand src) {
		return instrDestSrc(e, op::udiv, dest, src);
	}

	Instr *imod(EnginePtr e, Operand dest, Operand src) {
		return instrDestSrc(e, op::imod, dest, src);
	}

	Instr *umod(EnginePtr e, Operand dest, Operand src) {
		return instrDestSrc(e, op::umod, dest, src);
	}

	Instr *shl(EnginePtr e, Operand dest, Operand src) {
		if (src.size() != Size::sByte)
			throw InvalidValue(L"Size must be 1");
		return instrLoose(e, op::shl, dest, src);
	}

	Instr *shr(EnginePtr e, Operand dest, Operand src) {
		if (src.size() != Size::sByte)
			throw InvalidValue(L"Size must be 1");
		return instrLoose(e, op::shr, dest, src);
	}

	Instr *sar(EnginePtr e, Operand dest, Operand src) {
		if (src.size() != Size::sByte)
			throw InvalidValue(L"Size must be 1");
		return instrLoose(e, op::sar, dest, src);
	}

	Instr *icast(EnginePtr e, Operand dest, Operand src) {
		return instrLoose(e, op::icast, dest, src);
	}

	Instr *ucast(EnginePtr e, Operand dest, Operand src) {
		return instrLoose(e, op::ucast, dest, src);
	}

	Instr *fstp(EnginePtr e, Operand dest) {
		if (dest.type() == opRegister)
			throw InvalidValue(L"Can not store to register.");
		if (dest.size() != Size::sFloat)
			throw InvalidValue(L"Invalid size.");
		return instrDest(e, op::fstp, dest);
	}

	Instr *fistp(EnginePtr e, Operand dest) {
		if (dest.size() != Size::sInt)
			throw InvalidValue(L"Invalid size.");
		return instrDest(e, op::fistp, dest);
	}

	Instr *fld(EnginePtr e, Operand src) {
		if (src.type() == opRegister)
			throw InvalidValue(L"Can not load from register.");
		if (src.type() == opConstant)
			throw InvalidValue(L"Can not load from a constant.");
		if (src.size() != Size::sFloat)
			throw InvalidValue(L"Invalid size.");
		return instrSrc(e, op::fld, src);
	}

	Instr *fild(EnginePtr e, Operand src) {
		if (src.type() == opRegister)
			throw InvalidValue(L"Can not load from register.");
		if (src.type() == opConstant)
			throw InvalidValue(L"Can not load from a constant.");
		if (src.size() != Size::sInt)
			throw InvalidValue(L"Invalid size.");
		return instrSrc(e, op::fild, src);
	}

	Instr *faddp(EnginePtr e) {
		return instr(e, op::faddp);
	}

	Instr *fsubp(EnginePtr e) {
		return instr(e, op::fsubp);
	}

	Instr *fmulp(EnginePtr e) {
		return instr(e, op::fmulp);
	}

	Instr *fdivp(EnginePtr e) {
		return instr(e, op::fdivp);
	}

	Instr *fcompp(EnginePtr e) {
		return instr(e, op::fcompp);
	}

	Instr *fwait(EnginePtr e) {
		return instr(e, op::fwait);
	}

	Instr *retFloat(EnginePtr e, const Size &s) {
		Operand r = sizedReg(ptrA, s);
		return instrSrc(e, op::retFloat, r);
	}


	Instr *dat(EnginePtr e, Operand v) {
		switch (v.type()) {
		case opConstant:
		case opLabel:
		case opReference:
		case opObjReference:
			break;
		default:
			throw InvalidValue(L"Cannot store other than references, constants and labels in dat");
		}
		return instrSrc(e, op::dat, v);
	}

	Instr *prolog(EnginePtr e) {
		return instr(e, op::prolog);
	}

	Instr *epilog(EnginePtr e) {
		return instr(e, op::epilog);
	}

	Instr *begin(EnginePtr e, Part part) {
		return instrLoose(e, op::beginBlock, Operand(), part);
	}

	Instr *end(EnginePtr e, Part part) {
		return instrLoose(e, op::endBlock, Operand(), part);
	}

	Instr *threadLocal(EnginePtr e) {
		return instr(e, op::threadLocal);
	}

}
