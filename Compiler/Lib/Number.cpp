#include "stdafx.h"
#include "Number.h"

namespace storm {
	using namespace code;

	void ucast(InlineParams p) {
		if (!p.result->needed())
			return;
		*p.state->l << ucast(p.result->location(p.state).v, p.params->at(0));
	}

	void icast(InlineParams p) {
		if (!p.result->needed())
			return;
		*p.state->l << icast(p.result->location(p.state).v, p.params->at(0));
	}

	void numAdd(InlineParams p) {
		if (p.result->needed()) {
			Operand result = p.result->location(p.state).v;
			*p.state->l << mov(result, p.params->at(0));
			*p.state->l << add(result, p.params->at(1));
		}
	}

	void numSub(InlineParams p) {
		if (p.result->needed()) {
			Operand result = p.result->location(p.state).v;
			*p.state->l << mov(result, p.params->at(0));
			*p.state->l << sub(result, p.params->at(1));
		}
	}

	void numMul(InlineParams p) {
		if (p.result->needed()) {
			Operand result = p.result->location(p.state).v;
			*p.state->l << mov(result, p.params->at(0));
			*p.state->l << mul(result, p.params->at(1));
		}
	}

	void numIDiv(InlineParams p) {
		if (p.result->needed()) {
			Operand result = p.result->location(p.state).v;
			*p.state->l << mov(result, p.params->at(0));
			*p.state->l << idiv(result, p.params->at(1));
		}
	}

	void numUDiv(InlineParams p) {
		if (p.result->needed()) {
			Operand result = p.result->location(p.state).v;
			*p.state->l << mov(result, p.params->at(0));
			*p.state->l << udiv(result, p.params->at(1));
		}
	}

	void numIMod(InlineParams p) {
		if (p.result->needed()) {
			Operand result = p.result->location(p.state).v;
			*p.state->l << mov(result, p.params->at(0));
			*p.state->l << imod(result, p.params->at(1));
		}
	}

	void numUMod(InlineParams p) {
		if (p.result->needed()) {
			Operand result = p.result->location(p.state).v;
			*p.state->l << mov(result, p.params->at(0));
			*p.state->l << umod(result, p.params->at(1));
		}
	}

}
