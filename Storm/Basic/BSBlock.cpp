#include "stdafx.h"
#include "BSBlock.h"

namespace storm {

	bs::Block::Block() : parent(null) {}

	bs::Block::Block(Auto<Block> parent) : parent(parent.borrow()) {}

	void bs::Block::expr(Auto<Expr> e) {
		exprs.push_back(e);
	}

	Value bs::Block::result() {
		if (!exprs.empty())
			return exprs.back()->result();
		else
			return Value();
	}

	code::Listing bs::Block::code(code::Variable var) {
		using namespace code;
		Listing r;

		if (exprs.size() == 0)
			return r;

		for (nat i = 0; i < exprs.size() - 1; i++) {
			r << exprs[i]->code(Variable::invalid);
		}

		r << exprs[exprs.size() - 1]->code(var);

		return r;
	}

}
