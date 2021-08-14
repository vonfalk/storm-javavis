#include "stdafx.h"
#include "Number.h"

namespace storm {
	using namespace code;

	void ucast(InlineParams p) {
		if (!p.result->needed())
			return;
		*p.state->l << ucast(p.result->location(p.state), p.param(0));
	}

	void icast(InlineParams p) {
		if (!p.result->needed())
			return;
		*p.state->l << icast(p.result->location(p.state), p.param(0));
	}

	void numAdd(InlineParams p) {
		if (p.result->needed()) {
			Operand result = p.result->location(p.state);
			*p.state->l << mov(result, p.param(0));
			*p.state->l << add(result, p.param(1));
		}
	}

	void numSub(InlineParams p) {
		if (p.result->needed()) {
			Operand result = p.result->location(p.state);
			*p.state->l << mov(result, p.param(0));
			*p.state->l << sub(result, p.param(1));
		}
	}

	void numMul(InlineParams p) {
		if (p.result->needed()) {
			Operand result = p.result->location(p.state);
			*p.state->l << mov(result, p.param(0));
			*p.state->l << mul(result, p.param(1));
		}
	}

	void numIDiv(InlineParams p) {
		if (p.result->needed()) {
			Operand result = p.result->location(p.state);
			*p.state->l << mov(result, p.param(0));
			*p.state->l << idiv(result, p.param(1));
		}
	}

	void numUDiv(InlineParams p) {
		if (p.result->needed()) {
			Operand result = p.result->location(p.state);
			*p.state->l << mov(result, p.param(0));
			*p.state->l << udiv(result, p.param(1));
		}
	}

	void numIMod(InlineParams p) {
		if (p.result->needed()) {
			Operand result = p.result->location(p.state);
			*p.state->l << mov(result, p.param(0));
			*p.state->l << imod(result, p.param(1));
		}
	}

	void numUMod(InlineParams p) {
		if (p.result->needed()) {
			Operand result = p.result->location(p.state);
			*p.state->l << mov(result, p.param(0));
			*p.state->l << umod(result, p.param(1));
		}
	}

	void numINeg(InlineParams p) {
		if (p.result->needed()) {
			Operand result = p.result->location(p.state);
			// Note: it would be nice to use the 'neg' instruction for this.
			*p.state->l << mov(result, xConst(result.size(), 0));
			*p.state->l << sub(result, p.param(0));
		}
	}

	void numAnd(InlineParams p) {
		if (p.result->needed()) {
			Operand result = p.result->location(p.state);
			*p.state->l << mov(result, p.param(0));
			*p.state->l << band(result, p.param(1));
		}
	}

	void numOr(InlineParams p) {
		if (p.result->needed()) {
			Operand result = p.result->location(p.state);
			*p.state->l << mov(result, p.param(0));
			*p.state->l << bor(result, p.param(1));
		}
	}

	void numXor(InlineParams p) {
		if (p.result->needed()) {
			Operand result = p.result->location(p.state);
			*p.state->l << mov(result, p.param(0));
			*p.state->l << bxor(result, p.param(1));
		}
	}

	void numNot(InlineParams p) {
		if (p.result->needed()) {
			Operand result = p.result->location(p.state);
			*p.state->l << mov(result, p.param(0));
			*p.state->l << bnot(result);
		}
	}

	void numShl(InlineParams p) {
		if (p.result->needed()) {
			Operand result = p.result->location(p.state);
			*p.state->l << mov(result, p.param(0));
			*p.state->l << shl(result, p.param(1));
		}
	}

	void numShr(InlineParams p) {
		if (p.result->needed()) {
			Operand result = p.result->location(p.state);
			*p.state->l << mov(result, p.param(0));
			*p.state->l << shr(result, p.param(1));
		}
	}

	void numSar(InlineParams p) {
		if (p.result->needed()) {
			Operand result = p.result->location(p.state);
			*p.state->l << mov(result, p.param(0));
			*p.state->l << sar(result, p.param(1));
		}
	}

}
