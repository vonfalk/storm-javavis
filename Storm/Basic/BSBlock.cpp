#include "stdafx.h"
#include "BSBlock.h"
#include "BSVar.h"
#include "Exception.h"

namespace storm {

	bs::Block::Block(Auto<BSScope> scope) : scope(scope->child(capture(this))), parent(null) {}

	bs::Block::Block(Auto<Block> parent) : scope(parent->scope->child(capture(this))), parent(parent.borrow()) {}

	void bs::Block::expr(Auto<Expr> e) {
		exprs.push_back(e);
	}

	Value bs::Block::result() {
		if (!exprs.empty())
			return exprs.back()->result();
		else
			return Value();
	}

	void bs::Block::code(const GenState &state, GenResult &to) {
		using namespace code;

		if (exprs.size() == 0)
			return;

		code::Block block = state.frame.createChild(state.block);
		GenState subState = { state.to, state.frame, block };

		for (VarMap::const_iterator i = variables.begin(); i != variables.end(); ++i) {
			LocalVar *v = i->second.borrow();
			if (!v->param)
				v->var = state.frame.createVariable(block, v->result.size(), v->result.destructor());
		}

		state.to << begin(block);

		for (nat i = 0; i < exprs.size() - 1; i++) {
			GenResult s;
			exprs[i]->code(subState, s);
		}

		exprs[exprs.size() - 1]->code(subState, to);
		state.to << end(block);
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

}

