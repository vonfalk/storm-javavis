#include "stdafx.h"
#include "BSBlock.h"

namespace storm {

	bs::Block::Block(Auto<BSScope> scope) : scope(scope), parent(null) {}

	bs::Block::Block(Auto<Block> parent) : scope(parent->scope), parent(parent.borrow()) {}

	void bs::Block::expr(Auto<Expr> e) {
		exprs.push_back(e);
	}

	Value bs::Block::result() {
		if (!exprs.empty())
			return exprs.back()->result();
		else
			return Value();
	}

	void bs::Block::code(GenState state, code::Variable var) {
		using namespace code;

		if (exprs.size() == 0)
			return;

		code::Block block = state.frame.createChild(state.block);
		GenState subState = { state.to, state.frame, block };

		for (nat i = 0; i < exprs.size() - 1; i++) {
			exprs[i]->code(subState, Variable::invalid);
		}

		exprs[exprs.size() - 1]->code(subState, var);
	}

}
