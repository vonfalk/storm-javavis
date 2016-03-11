#pragma once
#include "Code.h"

namespace storm {

	/**
	 * Generate inline asm for number operations.
	 */

	// Cast unsigned value.
	void ucast(InlinedParams p);

	// Cast signed value.
	void icast(InlinedParams p);

	// Arithmetic.
	void numAdd(InlinedParams p);
	void numSub(InlinedParams p);
	void numMul(InlinedParams p);
	void numIdiv(InlinedParams p);
	void numUdiv(InlinedParams p);
	void numImod(InlinedParams p);
	void numUmod(InlinedParams p);

	// Compare
	template <code::CondFlag f>
	void numCmp(InlinedParams p) {
		if (p.result->needed()) {
			code::Value result = p.result->location(p.state).var();
			p.state->to << code::cmp(p.params[0], p.params[1]);
			p.state->to << code::setCond(result, f);
		}
	}

	// Get a relative value for the size of T
	inline code::Value tRel(Int i, code::Register reg) { return code::intRel(reg); }
	inline code::Value tRel(Nat i, code::Register reg) { return code::intRel(reg); }
	inline code::Value tRel(Byte i, code::Register reg) { return code::byteRel(reg); }
	inline code::Value tRel(Long i, code::Register reg) { return code::longRel(reg); }
	inline code::Value tRel(Word i, code::Register reg) { return code::longRel(reg); }

	// Get the value one for various types.
	inline code::Value tConst(Int i) { return code::intConst(i); }
	inline code::Value tConst(Nat i) { return code::natConst(i); }
	inline code::Value tConst(Byte i) { return code::byteConst(i); }
	inline code::Value tConst(Word i) { return code::wordConst(i); }
	inline code::Value tConst(Long i) { return code::longConst(i); }

	// Increase/decrease
	template <class T>
	void numInc(InlinedParams p) {
		p.state->to << mov(ptrA, p.params[0]);
		p.state->to << add(tRel(T(), ptrA), p.params[1]);
		if (p.result->needed())
			p.state->to << mov(p.result->location(p.state).var(), tRel(T(), ptrA));
	}

	template <class T>
	void numDec(InlinedParams p) {
		p.state->to << mov(ptrA, p.params[0]);
		p.state->to << sub(tRel(T(), ptrA), p.params[1]);
		if (p.result->needed())
			p.state->to << mov(p.result->location(p.state).var(), tRel(T(), ptrA));
	}

	template <class T>
	void numScale(InlinedParams p) {
		p.state->to << mov(ptrA, p.params[0]);
		p.state->to << mul(tRel(T(), ptrA), p.params[1]);
		if (p.result->needed())
			p.state->to << mov(p.result->location(p.state).var(), tRel(T(), ptrA));
	}

	template <class T>
	void numIDivScale(InlinedParams p) {
		p.state->to << mov(ptrA, p.params[0]);
		p.state->to << idiv(tRel(T(), ptrA), p.params[1]);
		if (p.result->needed())
			p.state->to << mov(p.result->location(p.state).var(), tRel(T(), ptrA));
	}

	template <class T>
	void numUDivScale(InlinedParams p) {
		p.state->to << mov(ptrA, p.params[0]);
		p.state->to << udiv(tRel(T(), ptrA), p.params[1]);
		if (p.result->needed())
			p.state->to << mov(p.result->location(p.state).var(), tRel(T(), ptrA));
	}

	template <class T>
	void numIModEq(InlinedParams p) {
		p.state->to << mov(ptrA, p.params[0]);
		p.state->to << imod(tRel(T(), ptrA), p.params[1]);
		if (p.result->needed())
			p.state->to << mov(p.result->location(p.state).var(), tRel(T(), ptrA));
	}

	template <class T>
	void numUModEq(InlinedParams p) {
		p.state->to << mov(ptrA, p.params[0]);
		p.state->to << umod(tRel(T(), ptrA), p.params[1]);
		if (p.result->needed())
			p.state->to << mov(p.result->location(p.state).var(), tRel(T(), ptrA));
	}

	template <class T>
	void numPrefixInc(InlinedParams p) {
		p.state->to << mov(ptrA, p.params[0]);
		p.state->to << add(tRel(T(), ptrA), tConst(T(1)));
		if (p.result->needed())
			p.state->to << mov(p.result->location(p.state).var(), tRel(T(), ptrA));
	}

	template <class T>
	void numPostfixInc(InlinedParams p) {
		p.state->to << mov(ptrA, p.params[0]);
		if (p.result->needed())
			p.state->to << mov(p.result->location(p.state).var(), tRel(T(), ptrA));
		p.state->to << add(tRel(T(), ptrA), tConst(T(1)));
	}

	template <class T>
	void numPrefixDec(InlinedParams p) {
		p.state->to << mov(ptrA, p.params[0]);
		p.state->to << sub(tRel(T(), ptrA), tConst(T(1)));
		if (p.result->needed())
			p.state->to << mov(p.result->location(p.state).var(), tRel(T(), ptrA));
	}

	template <class T>
	void numPostfixDec(InlinedParams p) {
		p.state->to << mov(ptrA, p.params[0]);
		if (p.result->needed())
			p.state->to << mov(p.result->location(p.state).var(), tRel(T(), ptrA));
		p.state->to << sub(tRel(T(), ptrA), tConst(T(1)));
	}


	template <class T>
	inline T CODECALL numMin(T a, T b) {
		return min(a, b);
	}

	template <class T>
	inline T CODECALL numMax(T a, T b) {
		return max(a, b);
	}

}
