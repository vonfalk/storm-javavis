#include "stdafx.h"
#include "BSBlock.h"
#include "BSVar.h"
#include "Exception.h"

namespace storm {

	bs::Block::Block(const Scope &scope)
		: lookup(CREATE(BlockLookup, engine(), this, scope.top)), scope(scope, lookup) {}

	bs::Block::Block(Par<Block> parent)
		: lookup(CREATE(BlockLookup, engine(), this, parent->scope.top)), scope(parent->scope, lookup) {}

	void bs::Block::code(const GenState &state, GenResult &to) {
		using namespace code;

		code::Block block = state.frame.createChild(state.block);

		for (VarMap::const_iterator i = variables.begin(); i != variables.end(); ++i) {
			LocalVar *v = i->second.borrow();
			if (!v->param)
				v->var = state.frame.createVariable(block, v->result.size(), v->result.destructor());
		}

		blockCode(state, to, block);
	}

	void bs::Block::blockCode(const GenState &state, GenResult &to, const code::Block &block) {
		state.to << begin(block);
		GenState subState = { state.to, state.frame, block };
		blockCode(subState, to);
		state.to << end(block);
	}

	void bs::Block::blockCode(const GenState &state, GenResult &to) {
		assert(("Implement me in a subclass!", false));
	}

	void bs::Block::add(Par<LocalVar> var) {
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


	bs::ExprBlock::ExprBlock(const Scope &scope) : Block(scope) {}

	bs::ExprBlock::ExprBlock(Par<Block> parent) : Block(parent) {}

	void bs::ExprBlock::expr(Par<Expr> expr) {
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

	/**
	 * Block lookup
	 */

	bs::BlockLookup::BlockLookup(Par<Block> o, NameLookup *prev) : block(o.borrow()), prev(prev) {}

	Named *bs::BlockLookup::find(const Name &name) {
		if (name.size() == 1)
			return block->variable(name[0]);
		else
			return null;
	}

	NameLookup *bs::BlockLookup::parent() const {
		return prev;
	}

}

