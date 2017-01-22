#include "stdafx.h"
#include "BSUnless.h"
#include "Exception.h"

namespace storm {

	bs::Unless::Unless(Par<Block> parent, Par<WeakCast> cast) : Block(cast->pos, parent), cast(cast) {
		init(null);
	}

	bs::Unless::Unless(Par<Block> parent, Par<WeakCast> cast, Par<SStr> name) : Block(cast->pos, parent), cast(cast) {
		init(name->v);
	}

	void bs::Unless::init(Auto<Str> name) {
		successBlock = CREATE(ExprBlock, this, pos, this);

		if (!name)
			name = cast->overwrite();
		if (!name)
			return;

		overwrite = CREATE(LocalVar, this, name->v, cast->result(), cast->pos);
		Block::add(overwrite);
	}

	void bs::Unless::fail(Par<Expr> expr) {
		if (expr->result() != noReturn())
			throw SyntaxError(pos, L"The block in 'until' must always do a 'return' or 'throw'!");
		failStmt = expr;
	}

	void bs::Unless::success(Par<Expr> expr) {
		successBlock->add(expr);
	}

	ExprResult bs::Unless::result() {
		return successBlock->result();
	}

	void bs::Unless::blockCode(Par<CodeGen> state, Par<CodeResult> r) {
		using namespace code;

		// Create the 'created' variable here. The extra scoping-time will not matter too much.
		if (overwrite)
			overwrite->create(state);

		Auto<CodeResult> cond = CREATE(CodeResult, this, boolType(engine()), state->block);
		cast->code(state, cond, overwrite);

		Label skipLbl = state->to.label();

		state->to << cmp(cond->location(state).var(), byteConst(0));
		state->to << jmp(skipLbl, ifNotEqual);

		Auto<CodeResult> failResult = CREATE(CodeResult, this);
		failStmt->code(state, failResult);
		// We have verified that 'failStmt' never returns, no worry about extra jumps!

		state->to << skipLbl;

		successBlock->code(state, r);
	}

	void bs::Unless::output(wostream &to) const {
		to << "unless (" << cast << ") " << failStmt << ";\n" << successBlock;
	}

}
