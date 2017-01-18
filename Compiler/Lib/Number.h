#pragma once
#include "Code.h"

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

	// Compare.
	template <code::CondFlag f>
	void numCmp(InlineParams p) {
		if (p.result->needed()) {
			code::Operand result = p.result->location(p.state).v;
			*p.state->to << code::cmp(p.params->at(0), p.params->at(1));
			*p.state->to << code::setCond(result, f);
		}
	}


	template <class T>
	inline T CODECALL numMin(T a, T b) {
		return min(a, b);
	}

	template <class T>
	inline T CODECALL numMax(T a, T b) {
		return max(a, b);
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

	// Increase/decrease
	template <class T>
	void numInc(InlineParams p) {
		*p.state->to << mov(code::ptrA, p.params->at(0));
		*p.state->to << add(tRel(T(), code::ptrA), p.params->at(1));
		if (p.result->needed())
			*p.state->to << mov(p.result->location(p.state).v, tRel(T(), code::ptrA));
	}

	template <class T>
	void numDec(InlineParams p) {
		*p.state->to << mov(code::ptrA, p.params->at(0));
		*p.state->to << sub(tRel(T(), code::ptrA), p.params->at(1));
		if (p.result->needed())
			*p.state->to << mov(p.result->location(p.state).v, tRel(T(), code::ptrA));
	}

	template <class T>
	void numScale(InlineParams p) {
		*p.state->to << mov(code::ptrA, p.params->at(0));
		*p.state->to << mul(tRel(T(), code::ptrA), p.params->at(1));
		if (p.result->needed())
			*p.state->to << mov(p.result->location(p.state).v, tRel(T(), code::ptrA));
	}

	template <class T>
	void numIDivScale(InlineParams p) {
		*p.state->to << mov(code::ptrA, p.params->at(0));
		*p.state->to << idiv(tRel(T(), code::ptrA), p.params->at(1));
		if (p.result->needed())
			*p.state->to << mov(p.result->location(p.state).v, tRel(T(), code::ptrA));
	}

	template <class T>
	void numUDivScale(InlineParams p) {
		*p.state->to << mov(code::ptrA, p.params->at(0));
		*p.state->to << udiv(tRel(T(), code::ptrA), p.params->at(1));
		if (p.result->needed())
			*p.state->to << mov(p.result->location(p.state).v, tRel(T(), code::ptrA));
	}

	template <class T>
	void numIModEq(InlineParams p) {
		*p.state->to << mov(code::ptrA, p.params->at(0));
		*p.state->to << imod(tRel(T(), code::ptrA), p.params->at(1));
		if (p.result->needed())
			*p.state->to << mov(p.result->location(p.state).v, tRel(T(), code::ptrA));
	}

	template <class T>
	void numUModEq(InlineParams p) {
		*p.state->to << mov(code::ptrA, p.params->at(0));
		*p.state->to << umod(tRel(T(), code::ptrA), p.params->at(1));
		if (p.result->needed())
			*p.state->to << mov(p.result->location(p.state).v, tRel(T(), code::ptrA));
	}

	template <class T>
	void numPrefixInc(InlineParams p) {
		*p.state->to << mov(code::ptrA, p.params->at(0));
		*p.state->to << add(tRel(T(), code::ptrA), tConst(T(1)));
		if (p.result->needed())
			*p.state->to << mov(p.result->location(p.state).v, tRel(T(), code::ptrA));
	}

	template <class T>
	void numPostfixInc(InlineParams p) {
		*p.state->to << mov(code::ptrA, p.params->at(0));
		if (p.result->needed())
			*p.state->to << mov(p.result->location(p.state).v, tRel(T(), code::ptrA));
		*p.state->to << add(tRel(T(), code::ptrA), tConst(T(1)));
	}

	template <class T>
	void numPrefixDec(InlineParams p) {
		*p.state->to << mov(code::ptrA, p.params->at(0));
		*p.state->to << sub(tRel(T(), code::ptrA), tConst(T(1)));
		if (p.result->needed())
			*p.state->to << mov(p.result->location(p.state).v, tRel(T(), code::ptrA));
	}

	template <class T>
	void numPostfixDec(InlineParams p) {
		*p.state->to << mov(code::ptrA, p.params->at(0));
		if (p.result->needed())
			*p.state->to << mov(p.result->location(p.state).v, tRel(T(), code::ptrA));
		*p.state->to << sub(tRel(T(), code::ptrA), tConst(T(1)));
	}

	template <class T>
	void numAssign(InlineParams p) {
		*p.state->to << mov(code::ptrA, p.params->at(0));
		*p.state->to << mov(tRel(T(), code::ptrA), p.params->at(1));
		if (p.result->needed()) {
			if (p.result->type().ref) {
				// Try to suggest params[0], since we already have a ref to the value there.
				if (!p.result->suggest(p.state, p.params->at(0)))
					*p.state->to << mov(p.result->location(p.state).v, code::ptrA);
			} else {
				// Try to suggest params[1], since we already have the value there.
				if (!p.result->suggest(p.state, p.params->at(1)))
					*p.state->to << mov(p.result->location(p.state).v, code::ptrA);
			}
		}
	}

	template <class T>
	void numCopyCtor(InlineParams p) {
		*p.state->to << mov(code::ptrC, p.params->at(1));
		*p.state->to << mov(code::ptrA, p.params->at(0));
		*p.state->to << mov(tRel(T(), code::ptrA), tRel(T(), code::ptrC));
	}

	template <class T>
	void numInit(InlineParams p) {
		*p.state->to << mov(code::ptrA, p.params->at(0));
		*p.state->to << mov(tRel(T(), code::ptrA), tConst(T(0)));
	}

}
