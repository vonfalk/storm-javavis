#include "stdafx.h"
#include "Number.h"

namespace storm {
	using namespace code;

	void ucast(InlineParams p) {
		if (!p.result->needed())
			return;
		*p.state->to << ucast(p.result->location(p.state).v, p.params->at(0));
	}

	void icast(InlineParams p) {
		if (!p.result->needed())
			return;
		*p.state->to << icast(p.result->location(p.state).v, p.params->at(0));
	}

	void numAdd(InlineParams p) {
		if (p.result->needed()) {
			code::Operand result = p.result->location(p.state).v;
			*p.state->to << mov(result, p.params->at(0));
			*p.state->to << add(result, p.params->at(1));
		}
	}

	void numSub(InlineParams p) {
		if (p.result->needed()) {
			code::Operand result = p.result->location(p.state).v;
			*p.state->to << mov(result, p.params->at(0));
			*p.state->to << sub(result, p.params->at(1));
		}
	}

	void numMul(InlineParams p) {
		if (p.result->needed()) {
			code::Operand result = p.result->location(p.state).v;
			*p.state->to << mov(result, p.params->at(0));
			*p.state->to << mul(result, p.params->at(1));
		}
	}

	void numIDiv(InlineParams p) {
		if (p.result->needed()) {
			code::Operand result = p.result->location(p.state).v;
			*p.state->to << mov(result, p.params->at(0));
			*p.state->to << idiv(result, p.params->at(1));
		}
	}

	void numUDiv(InlineParams p) {
		if (p.result->needed()) {
			code::Operand result = p.result->location(p.state).v;
			*p.state->to << mov(result, p.params->at(0));
			*p.state->to << udiv(result, p.params->at(1));
		}
	}

	void numIMod(InlineParams p) {
		if (p.result->needed()) {
			code::Operand result = p.result->location(p.state).v;
			*p.state->to << mov(result, p.params->at(0));
			*p.state->to << imod(result, p.params->at(1));
		}
	}

	void numUMod(InlineParams p) {
		if (p.result->needed()) {
			code::Operand result = p.result->location(p.state).v;
			*p.state->to << mov(result, p.params->at(0));
			*p.state->to << umod(result, p.params->at(1));
		}
	}

}
