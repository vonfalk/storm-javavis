#include "stdafx.h"
#include "BSBlock.h"
#include "BSVar.h"
#include "Exception.h"

namespace storm {

	bs::Block::Block(Auto<BSScope> scope) : scope(scope->child(capture(this))), parent(null) {}

	bs::Block::Block(Auto<Block> parent) : scope(parent->scope->child(capture(this))), parent(parent.borrow()) {}

	void bs::Block::code(const GenState &state, GenResult &to) {
		using namespace code;

		code::Block block = state.frame.createChild(state.block);
		GenState subState = { state.to, state.frame, block };

		for (VarMap::const_iterator i = variables.begin(); i != variables.end(); ++i) {
			LocalVar *v = i->second.borrow();
			if (!v->param)
				v->var = state.frame.createVariable(block, v->result.size(), v->result.destructor());
		}

		state.to << begin(block);

		blockCode(subState, to);

		state.to << end(block);
	}

	void bs::Block::blockCode(const GenState &state, GenResult &to) {
		assert(("Implement me!", false));
	}

	void bs::Block::add(Auto<LocalVar> var) {
		LocalVar *old = variable(var->name);
		if (old != null)
			throw TypeError(old->pos, L"The variable " + old->name + L" is already defined.");
		variables.insert(make_pair(var->name, var));
	}

	bs::LocalVar *bs::Block::variable(const String &name) {
		VarMap::const_iterator i = variables.find(name);
		if (i != variables.end())
			return i->second.borrow();
		else
			return null;
	}


	bs::ExprBlock::ExprBlock(Auto<BSScope> scope) : Block(scope) {}

	bs::ExprBlock::ExprBlock(Auto<Block> parent) : Block(parent) {}

	void bs::ExprBlock::expr(Auto<Expr> expr) {
		exprs.push_back(expr);
	}

	Value bs::ExprBlock::result() {
		if (!exprs.empty())
			return exprs.back()->result();
		else
			return Value();
	}

	void bs::ExprBlock::code(const GenState &state, GenResult &to) {
		if (!exprs.empty())
			Block::code(state, to);
	}

	void bs::ExprBlock::blockCode(const GenState &state, GenResult &to) {
		for (nat i = 0; i < exprs.size() - 1; i++) {
			GenResult s;
			exprs[i]->code(state, s);
		}

		exprs[exprs.size() - 1]->code(state, to);
	}

}

