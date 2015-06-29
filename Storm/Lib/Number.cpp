#include "stdafx.h"
#include "Number.h"
#include "CodeGen.h"

namespace storm {
	using namespace code;

	void ucast(InlinedParams p) {
		if (!p.result->needed())
			return;
		p.state->to << ucast(p.result->location(p.state).var(), p.params[0]);
	}

	void icast(InlinedParams p) {
		if (!p.result->needed())
			return;
		p.state->to << icast(p.result->location(p.state).var(), p.params[0]);
	}

	void numAdd(InlinedParams p) {
		if (p.result->needed()) {
			code::Value result = p.result->location(p.state).var();
			p.state->to << mov(result, p.params[0]);
			p.state->to << add(result, p.params[1]);
		}
	}

	void numSub(InlinedParams p) {
		if (p.result->needed()) {
			code::Value result = p.result->location(p.state).var();
			p.state->to << mov(result, p.params[0]);
			p.state->to << sub(result, p.params[1]);
		}
	}

	void numMul(InlinedParams p) {
		if (p.result->needed()) {
			code::Value result = p.result->location(p.state).var();
			p.state->to << mov(result, p.params[0]);
			p.state->to << mul(result, p.params[1]);
		}
	}

	void numIdiv(InlinedParams p) {
		if (p.result->needed()) {
			code::Value result = p.result->location(p.state).var();
			p.state->to << mov(result, p.params[0]);
			p.state->to << idiv(result, p.params[1]);
		}
	}

	void numUdiv(InlinedParams p) {
		if (p.result->needed()) {
			code::Value result = p.result->location(p.state).var();
			p.state->to << mov(result, p.params[0]);
			p.state->to << udiv(result, p.params[1]);
		}
	}

	void numImod(InlinedParams p) {
		if (p.result->needed()) {
			code::Value result = p.result->location(p.state).var();
			p.state->to << mov(result, p.params[0]);
			p.state->to << imod(result, p.params[1]);
		}
	}

	void numUmod(InlinedParams p) {
		if (p.result->needed()) {
			code::Value result = p.result->location(p.state).var();
			p.state->to << mov(result, p.params[0]);
			p.state->to << umod(result, p.params[1]);
		}
	}

}
