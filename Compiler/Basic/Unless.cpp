#include "stdafx.h"
#include "Unless.h"
#include "Compiler/Exception.h"

namespace storm {
	namespace bs {

		Unless::Unless(Block *parent, WeakCast *cast) : Block(cast->pos, parent), cast(cast) {
			init(null);
		}

		Unless::Unless(Block *parent, WeakCast *cast, syntax::SStr *name) : Block(cast->pos, parent), cast(cast) {
			init(name->v);
		}

		void Unless::init(Str *name) {
			successBlock = new (this) ExprBlock(pos, this);

			if (!name)
				name = cast->overwrite();
			if (!name)
				return;

			overwrite = new (this) LocalVar(name, cast->result(), cast->pos, false);
			Block::add(overwrite);
		}

		void Unless::fail(Expr *expr) {
			if (expr->result() != noReturn())
				throw SyntaxError(pos, L"The block in 'until' must always do a 'return' or 'throw'!");
			failStmt = expr;
		}

		void Unless::success(Expr *expr) {
			successBlock->add(expr);
		}

		ExprResult Unless::result() {
			return successBlock->result();
		}

		void Unless::blockCode(CodeGen *state, CodeResult *r) {
			using namespace code;

			// Create the 'created' variable here. The extra scoping-time will not matter too much.
			if (overwrite)
				overwrite->create(state);

			CodeResult *cond = new (this) CodeResult(Value(StormInfo<Bool>::type(engine())), state->block);
			cast->code(state, cond, overwrite);

			Label skipLbl = state->to->label();

			*state->to << cmp(cond->location(state).v, byteConst(0));
			*state->to << jmp(skipLbl, ifNotEqual);

			CodeResult *failResult = CREATE(CodeResult, this);
			failStmt->code(state, failResult);
			// We have verified that 'failStmt' never returns, no worry about extra jumps!

			*state->to << skipLbl;

			successBlock->code(state, r);
		}

		void Unless::toS(StrBuf *to) const {
			*to << "unless (" << cast << ") " << failStmt << ";\n" << successBlock;
		}


	}
}
