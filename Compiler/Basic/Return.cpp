#include "stdafx.h"
#include "Return.h"
#include "Function.h"
#include "Cast.h"
#include "Compiler/Exception.h"

namespace storm {
	namespace bs {

		Return::Return(SrcPos pos, Block *block) : Expr(pos), returnType(findParentType(pos, block)) {
			if (returnType != Value()) {
				throw SyntaxError(pos, L"Trying to return a value from a void function.");
			}
		}

		Return::Return(SrcPos pos, Block *block, Expr *expr) : Expr(pos), returnType(findParentType(pos, block)) {
			this->expr = expectCastTo(expr, returnType, block->scope);
		}

		ExprResult Return::result() {
			return noReturn();
		}

		void Return::code(CodeGen *state, CodeResult *r) {
			if (returnType == Value())
				voidCode(state);
			else
				valueCode(state);
		}

		void Return::voidCode(CodeGen *state) {
			using namespace code;
			*state->l << fnRet();
		}

		void Return::valueCode(CodeGen *state) {
			using namespace code;

			// TODO? Check the refness of the casted expr?
			CodeResult *r = new (this) CodeResult(returnType, state->block);
			expr->code(state, r);

			VarInfo rval = r->location(state);
			state->returnValue(rval.v);
		}

		Value Return::findParentType(SrcPos pos, Block *block) {
			BlockLookup *lookup = block->lookup;

			while (lookup) {
				Block *block = lookup->block;
				if (FnBody *root = as<FnBody>(block))
					return root->type;

				lookup = as<BlockLookup>(lookup->parent());
			}

			throw SyntaxError(pos, L"A return statement is not located inside a function.");
		}

		void Return::toS(StrBuf *to) const {
			*to << S("return");
			if (expr)
				*to << S(" ") << expr;
		}

	}
}
