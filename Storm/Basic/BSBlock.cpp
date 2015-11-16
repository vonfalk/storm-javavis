#include "stdafx.h"
#include "BSBlock.h"
#include "BSVar.h"
#include "Exception.h"

namespace storm {

	bs::Block::Block(const Scope &scope)
		: lookup(CREATE(BlockLookup, engine(), this, scope.top)), scope(scope, lookup) {}

	bs::Block::Block(Par<Block> parent)
		: lookup(CREATE(BlockLookup, engine(), this, parent->scope.top)), scope(parent->scope, lookup) {}

	void bs::Block::code(Par<CodeGen> state, Par<CodeResult> to) {
		using namespace code;

		code::Block block = state->frame.createChild(state->frame.last(state->block.v));
		Auto<CodeGen> child = state->child(block);

		for (VarMap::const_iterator i = variables.begin(), end = variables.end(); i != end; ++i) {
			i->second->create(child);
		}

		blockCode(state, to, block);

		// May be delayed...
		if (to->needed())
			to->location(state).created(state);
	}

	void bs::Block::blockCode(Par<CodeGen> state, Par<CodeResult> to, const code::Block &block) {
		state->to << begin(block);
		Auto<CodeGen> subState = state->child(block);
		blockCode(subState, to);
		state->to << end(block);
	}

	void bs::Block::blockCode(Par<CodeGen> state, Par<CodeResult> to) {
		assert(false, "Implement me in a subclass!");
	}

	void bs::Block::add(Par<LocalVar> var) {
		VarMap::const_iterator i = variables.find(var->name);
		if (i != variables.end())
			throw TypeError(i->second->pos, L"The variable " + var->name + L" is already defined.");
		variables.insert(make_pair(var->name, var));
	}

	void bs::Block::liftVars(Par<Block> from) {
		assert(isParentTo(from), L"'liftVars' can only be used to move variables from child to parent, one step.");

		for (VarMap::const_iterator i = from->variables.begin(); i != from->variables.end(); ++i) {
			add(i->second);
		}

		from->variables.clear();
	}

	bool bs::Block::isParentTo(Par<Block> x) {
		return x->lookup->parent() == lookup.borrow();
	}

	bs::LocalVar *bs::Block::variableQ(const String &name) {
		VarMap::const_iterator i = variables.find(name);
		if (i != variables.end())
			return i->second.ret();

		BlockLookup *parent = as<BlockLookup>(lookup->parent());
		if (parent)
			return parent->block->variableQ(name);

		return null;
	}


	bs::ExprBlock::ExprBlock(const Scope &scope) : Block(scope) {}

	bs::ExprBlock::ExprBlock(Par<Block> parent) : Block(parent) {}

	void bs::ExprBlock::add(Par<Expr> expr) {
		exprs.push_back(expr);
	}

	Value bs::ExprBlock::result() {
		if (!exprs.empty())
			return exprs.back()->result();
		else
			return Value();
	}

	void bs::ExprBlock::code(Par<CodeGen> state, Par<CodeResult> to) {
		if (!exprs.empty())
			Block::code(state, to);
	}

	void bs::ExprBlock::blockCode(Par<CodeGen> state, Par<CodeResult> to) {
		for (nat i = 0; i < exprs.size() - 1; i++) {
			Auto<CodeResult> s = CREATE(CodeResult, this);
			exprs[i]->code(state, s);
		}

		exprs[exprs.size() - 1]->code(state, to);
	}

	Int bs::ExprBlock::castPenalty(Value to) {
		if (exprs.empty())
			return -1;
		return exprs.back()->castPenalty(to);
	}

	void bs::ExprBlock::output(wostream &to) const {
		to << L"{" << endl;
		{
			Indent i(to);
			for (nat i = 0; i < exprs.size(); i++) {
				to << exprs[i] << L";" << endl;
			}
		}
		to << L"}";
	}

	/**
	 * Block lookup
	 */

	bs::BlockLookup::BlockLookup(Par<Block> o, NameLookup *prev) : block(o.borrow()) {
		parentLookup = prev;
	}

	Named *bs::BlockLookup::findW(Par<NamePart> part) {
		if (part->params.empty())
			return block->variableQ(part->name);

		return null;
	}

}

