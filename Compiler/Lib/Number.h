#pragma once
#include "Code.h"
#include "Function.h"

namespace storm {

	/**
	 * Implementation of operations shared between the number type implementations.
	 */

	// Cast unsigned value.
	void ucast(InlineParams p);

	// Cast signed value.
	void icast(InlineParams p);

	// Arithmetic.
	void numAdd(InlineParams p);
	void numSub(InlineParams p);
	void numMul(InlineParams p);
	void numIDiv(InlineParams p);
	void numUDiv(InlineParams p);
	void numIMod(InlineParams p);
	void numUMod(InlineParams p);
	void numINeg(InlineParams p);

	// Bitwise operations.
	void numAnd(InlineParams p);
	void numOr(InlineParams p);
	void numXor(InlineParams p);
	void numNot(InlineParams p);

	// Compare.
	template <code::CondFlag f>
	void numCmp(InlineParams p) {
		if (p.result->needed()) {
			code::Operand result = p.result->location(p.state).v;
			*p.state->l << code::cmp(p.param(0), p.param(1));
			*p.state->l << code::setCond(result, f);
		}
	}


	// Get a relative value for the size of T
	inline code::Operand tRel(Int i, code::Reg reg) { return code::intRel(reg, Offset()); }
	inline code::Operand tRel(Nat i, code::Reg reg) { return code::intRel(reg, Offset()); }
	inline code::Operand tRel(Byte i, code::Reg reg) { return code::byteRel(reg, Offset()); }
	inline code::Operand tRel(Long i, code::Reg reg) { return code::longRel(reg, Offset()); }
	inline code::Operand tRel(Word i, code::Reg reg) { return code::longRel(reg, Offset()); }

	// Get the value one for various types.
	inline code::Operand tConst(Int i) { return code::intConst(i); }
	inline code::Operand tConst(Nat i) { return code::natConst(i); }
	inline code::Operand tConst(Byte i) { return code::byteConst(i); }
	inline code::Operand tConst(Word i) { return code::wordConst(i); }
	inline code::Operand tConst(Long i) { return code::longConst(i); }

	// Get the less-than comparision for various types.
	inline code::CondFlag tLess(Int i) { return code::ifLess; }
	inline code::CondFlag tLess(Nat i) { return code::ifBelow; }
	inline code::CondFlag tLess(Byte i) { return code::ifBelow; }
	inline code::CondFlag tLess(Word i) { return code::ifBelow; }
	inline code::CondFlag tLess(Long i) { return code::ifLess; }

	// Increase/decrease
	template <class T>
	void numInc(InlineParams p) {
		p.allocRegs(0);
		code::Reg dest = p.regParam(0);

		*p.state->l << add(tRel(T(), dest), p.param(1));
		if (p.result->needed())
			*p.state->l << mov(p.result->location(p.state).v, tRel(T(), dest));
	}

	template <class T>
	void numDec(InlineParams p) {
		p.allocRegs(0);
		code::Reg dest = p.regParam(0);

		*p.state->l << sub(tRel(T(), dest), p.param(1));
		if (p.result->needed())
			*p.state->l << mov(p.result->location(p.state).v, tRel(T(), dest));
	}

	template <class T>
	void numScale(InlineParams p) {
		p.allocRegs(0);
		code::Reg dest = p.regParam(0);

		*p.state->l << mul(tRel(T(), dest), p.param(1));
		if (p.result->needed())
			*p.state->l << mov(p.result->location(p.state).v, tRel(T(), dest));
	}

	template <class T>
	void numIDivScale(InlineParams p) {
		p.allocRegs(0);
		code::Reg dest = p.regParam(0);

		*p.state->l << idiv(tRel(T(), dest), p.param(1));
		if (p.result->needed())
			*p.state->l << mov(p.result->location(p.state).v, tRel(T(), dest));
	}

	template <class T>
	void numUDivScale(InlineParams p) {
		p.allocRegs(0);
		code::Reg dest = p.regParam(0);

		*p.state->l << udiv(tRel(T(), dest), p.param(1));
		if (p.result->needed())
			*p.state->l << mov(p.result->location(p.state).v, tRel(T(), dest));
	}

	template <class T>
	void numIModEq(InlineParams p) {
		p.allocRegs(0);
		code::Reg dest = p.regParam(0);

		*p.state->l << imod(tRel(T(), dest), p.param(1));
		if (p.result->needed())
			*p.state->l << mov(p.result->location(p.state).v, tRel(T(), dest));
	}

	template <class T>
	void numUModEq(InlineParams p) {
		p.allocRegs(0);
		code::Reg dest = p.regParam(0);

		*p.state->l << umod(tRel(T(), dest), p.param(1));
		if (p.result->needed())
			*p.state->l << mov(p.result->location(p.state).v, tRel(T(), dest));
	}

	template <class T>
	void numPrefixInc(InlineParams p) {
		p.allocRegs(0);
		code::Reg dest = p.regParam(0);

		*p.state->l << add(tRel(T(), dest), tConst(T(1)));
		if (p.result->needed())
			*p.state->l << mov(p.result->location(p.state).v, tRel(T(), dest));
	}

	template <class T>
	void numPostfixInc(InlineParams p) {
		p.allocRegs(0);
		code::Reg dest = p.regParam(0);

		if (p.result->needed())
			*p.state->l << mov(p.result->location(p.state).v, tRel(T(), dest));
		*p.state->l << add(tRel(T(), dest), tConst(T(1)));
	}

	template <class T>
	void numPrefixDec(InlineParams p) {
		p.allocRegs(0);
		code::Reg dest = p.regParam(0);

		*p.state->l << sub(tRel(T(), dest), tConst(T(1)));
		if (p.result->needed())
			*p.state->l << mov(p.result->location(p.state).v, tRel(T(), dest));
	}

	template <class T>
	void numPostfixDec(InlineParams p) {
		p.allocRegs(0);
		code::Reg dest = p.regParam(0);

		if (p.result->needed())
			*p.state->l << mov(p.result->location(p.state).v, tRel(T(), dest));
		*p.state->l << sub(tRel(T(), dest), tConst(T(1)));
	}

	template <class T>
	void numAssign(InlineParams p) {
		p.allocRegs(0);
		code::Reg dest = p.regParam(0);

		*p.state->l << mov(tRel(T(), dest), p.param(1));
		if (p.result->needed()) {
			if (p.result->type().ref) {
				// Try to suggest params[0], since we already have a ref to the value there.
				if (!p.result->suggest(p.state, p.originalParam(0)))
					*p.state->l << mov(p.result->location(p.state).v, dest);
			} else {
				// Try to suggest params[1], since we already have the value there.
				if (!p.result->suggest(p.state, p.param(1)))
					*p.state->l << mov(p.result->location(p.state).v, dest);
			}
		}
	}

	template <class T>
	void numAndEq(InlineParams p) {
		p.allocRegs(0);
		code::Reg dest = p.regParam(0);

		*p.state->l << band(tRel(T(), dest), p.param(1));
		if (p.result->needed())
			*p.state->l << mov(p.result->location(p.state).v, tRel(T(), dest));
	}

	template <class T>
	void numOrEq(InlineParams p) {
		p.allocRegs(0);
		code::Reg dest = p.regParam(0);

		*p.state->l << bor(tRel(T(), dest), p.param(1));
		if (p.result->needed())
			*p.state->l << mov(p.result->location(p.state).v, tRel(T(), dest));
	}

	template <class T>
	void numXorEq(InlineParams p) {
		p.allocRegs(0);
		code::Reg dest = p.regParam(0);

		*p.state->l << bxor(tRel(T(), dest), p.param(1));
		if (p.result->needed())
			*p.state->l << mov(p.result->location(p.state).v, tRel(T(), dest));
	}

	template <class T>
	void numCopyCtor(InlineParams p) {
		p.allocRegs(0, 1);
		*p.state->l << mov(tRel(T(), p.regParam(0)), tRel(T(), p.regParam(1)));
	}

	template <class T>
	void numInit(InlineParams p) {
		p.allocRegs(0);
		code::Reg dest = p.regParam(0);

		*p.state->l << mov(tRel(T(), dest), tConst(T(0)));
	}

	template <class T>
	void numMin(InlineParams p) {
		using namespace code;

		if (!p.result->needed())
			return;

		Label lbl = p.state->l->label();
		Label end = p.state->l->label();
		p.allocRegs(0, 1);
		Reg tmp1 = p.regParam(0);
		Reg tmp2 = p.regParam(1);

		Operand result = p.result->location(p.state).v;
		*p.state->l << cmp(tmp1, tmp2);
		*p.state->l << jmp(lbl, tLess(T()));
		*p.state->l << mov(result, tmp2);
		*p.state->l << jmp(end);
		*p.state->l << lbl;
		*p.state->l << mov(result, tmp1);
		*p.state->l << end;
	}

	template <class T>
	void numMax(InlineParams p) {
		using namespace code;

		if (!p.result->needed())
			return;

		Label lbl = p.state->l->label();
		Label end = p.state->l->label();
		p.allocRegs(0, 1);
		Reg tmp1 = p.regParam(0);
		Reg tmp2 = p.regParam(1);

		Operand result = p.result->location(p.state).v;
		*p.state->l << cmp(tmp1, tmp2);
		*p.state->l << jmp(lbl, tLess(T()));
		*p.state->l << mov(result, tmp1);
		*p.state->l << jmp(end);
		*p.state->l << lbl;
		*p.state->l << mov(result, tmp2);
		*p.state->l << end;
	}

	template <class T>
	void numDelta(InlineParams p) {
		using namespace code;

		if (!p.result->needed())
			return;

		Label lbl = p.state->l->label();
		Label end = p.state->l->label();
		p.allocRegs(0, 1);
		Reg tmp1 = p.regParam(0);
		Reg tmp2 = p.regParam(1);

		Operand result = p.result->location(p.state).v;
		*p.state->l << cmp(tmp1, tmp2);
		*p.state->l << jmp(lbl, tLess(T()));
		*p.state->l << sub(tmp1, tmp2);
		*p.state->l << mov(result, tmp1);
		*p.state->l << jmp(end);
		*p.state->l << lbl;
		*p.state->l << sub(tmp2, tmp1);
		*p.state->l << mov(result, tmp2);
		*p.state->l << end;
	}

}
