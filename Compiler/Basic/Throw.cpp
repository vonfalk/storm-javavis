#include "stdafx.h"
#include "Throw.h"
#include "Compiler/Type.h"
#include "Compiler/Engine.h"
#include "Compiler/Exception.h"

namespace storm {
	namespace bs {

		Throw::Throw(SrcPos pos, Expr *expr) : Expr(pos), expr(expr) {
			Type *exType = StormInfo<Exception>::type(engine());
			Value eType = expr->result().type();
			if (!eType.type || !eType.type->isA(exType)) {
				Str *msg = TO_S(this, S("Only classes inheriting from ") << exType->identifier() << S(" may be thrown."));
				throw new (this) SyntaxError(pos, msg);
			}
		}

		ExprResult Throw::result() {
			return noReturn();
		}

		void Throw::code(CodeGen *state, CodeResult *r) {
			using namespace code;

			CodeResult *res = new (this) CodeResult(expr->result().type().asRef(false), state->block);
			expr->code(state, res);

			code::Var val = res->location(state);
			if (res->type().ref)
				*state->l << fnParamRef(engine().ptrDesc(), val);
			else
				*state->l << fnParam(engine().ptrDesc(), val);
			*state->l << fnCall(engine().ref(builtin::throwException), false);

			// This function never returns.
		}

		void Throw::toS(StrBuf *to) const {
			*to << S("throw ") << expr;
		}

	}
}
