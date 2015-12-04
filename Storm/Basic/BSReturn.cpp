#include "stdafx.h"
#include "BSReturn.h"
#include "BSFunction.h"
#include "BSAutocast.h"
#include "Exception.h"

namespace storm {

	bs::Return::Return(SrcPos pos, Par<Block> block) : returnType(findParentType(pos, block)) {
		if (returnType != Value()) {
			throw SyntaxError(pos, L"Trying to return a value from a void function.");
		}
	}

	bs::Return::Return(SrcPos pos, Par<Block> block, Par<Expr> expr) : returnType(findParentType(pos, block)) {
		this->expr = expectCastTo(expr, returnType);
	}

	Value bs::Return::result() {
		TODO(L"Return no-return value.");
		return Value();
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

		if (returnType.returnInReg()) {
			l << mov(asSize(ptrA, returnType.size()), rval.var());
			if (returnType.refcounted())
				l << code::addRef(ptrA);

			l << epilog();
			l << ret(returnType.retVal());
		} else {
			assert(false, L"Returning values can not be implemented yet!");
		}
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

}
