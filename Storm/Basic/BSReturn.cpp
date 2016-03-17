#include "stdafx.h"
#include "BSReturn.h"
#include "BSFunction.h"
#include "BSAutocast.h"
#include "Exception.h"

namespace storm {

	bs::Return::Return(SrcPos pos, Par<Block> block) : Expr(pos), returnType(findParentType(pos, block)) {
		if (returnType != Value()) {
			throw SyntaxError(pos, L"Trying to return a value from a void function.");
		}
	}

	bs::Return::Return(SrcPos pos, Par<Block> block, Par<Expr> expr) : Expr(pos), returnType(findParentType(pos, block)) {
		this->expr = expectCastTo(expr, returnType);
	}

	ExprResult bs::Return::result() {
		return noReturn();
	}

	void bs::Return::code(Par<CodeGen> state, Par<CodeResult> r) {
		if (returnType == Value())
			voidCode(state);
		else
			valueCode(state);
	}

	void bs::Return::voidCode(Par<CodeGen> state) {
		using namespace code;
		state->to << epilog();
		state->to << ret(retVoid());
	}

	void bs::Return::valueCode(Par<CodeGen> state) {
		using namespace code;
		Listing &l = state->to;

		// TODO? Check the refness of the casted expr?
		Auto<CodeResult> r = CREATE(CodeResult, this, returnType, state->block.v);
		expr->code(state, r);

		VarInfo rval = r->location(state);
		state->returnValue(rval.var());
	}

	Value bs::Return::findParentType(SrcPos pos, Par<Block> block) {
		BlockLookup *lookup = block->lookup.borrow();

		while (lookup) {
			Block *block = lookup->block;
			if (FnBody *root = as<FnBody>(block))
				return root->type;

			lookup = as<BlockLookup>(lookup->parent());
		}

		throw SyntaxError(pos, L"A return statement is not located inside a function.");
	}

	void bs::Return::output(wostream &to) const {
		to << L"return";
		if (expr)
			to << L" " << expr;
	}

}
