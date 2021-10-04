#include "stdafx.h"
#include "Return.h"
#include "Function.h"
#include "Cast.h"
#include "Compiler/Exception.h"

namespace storm {
	namespace bs {

		Return::Return(SrcPos pos, Block *block) : Expr(pos), returnType(findParentType(pos, block)) {
			if (returnType != Value()) {
				throw new (this) SyntaxError(pos, TO_S(this, S("Trying to return void from a function that returns ") << returnType));
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

			// If we can get the result as a reference, do that as we avoid a copy. The exception
			// is built-in types, as we can copy them cheaply.
			Value type = returnType;
			if (!type.isPrimitive() && expr->result().type().ref)
				type = returnType.asRef();

			CodeResult *r = new (this) CodeResult(type, state->block);
			expr->code(state, r);

			if (type.ref)
				*state->l << fnRetRef(r->location(state));
			else
				state->returnValue(r->location(state));
		}

		Value Return::findParentType(SrcPos pos, Block *block) {
			BlockLookup *lookup = block->lookup;

			while (lookup) {
				Block *block = lookup->block;
				if (FnBody *root = as<FnBody>(block))
					return root->type;

				lookup = as<BlockLookup>(lookup->parent());
			}

			throw new (this) SyntaxError(pos, S("A return statement is not located inside a function."));
		}

		void Return::toS(StrBuf *to) const {
			*to << S("return");
			if (expr)
				*to << S(" ") << expr;
		}

	}
}
