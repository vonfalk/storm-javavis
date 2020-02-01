#include "stdafx.h"
#include "For.h"
#include "Cast.h"

namespace storm {
	namespace bs {

		For::For(SrcPos pos, Block *parent) : Block(pos, parent) {}

		void For::test(Expr *e) {
			testExpr = expectCastTo(e, Value(StormInfo<Bool>::type(engine())), scope);
		}

		void For::update(Expr *e) {
			updateExpr = e;
		}

		void For::body(Expr *e) {
			bodyExpr = e;
		}

		ExprResult For::result() {
			return ExprResult();
		}

		void For::blockCode(CodeGen *s, CodeResult *to, code::Block block) {
			using namespace code;

			Label begin = s->l->label();
			Label end = s->l->label();
			CodeGen *subState = s->child(block);

			*s->l << begin;
			*s->l << code::begin(block);

			// Put this outside the current block, so we can use it later.
			CodeResult *testResult = new (this) CodeResult(Value(StormInfo<Bool>::type(engine())), s->block);
			testExpr->code(subState, testResult);
			code::Var r = testResult->location(subState);
			*s->l << cmp(r, byteConst(0));
			*s->l << jmp(end, ifEqual);

			CodeResult *bodyResult = CREATE(CodeResult, this);
			bodyExpr->code(subState, bodyResult);

			CodeResult *updateResult = CREATE(CodeResult, this);
			updateExpr->code(subState, updateResult);

			// We may not skip the 'end'.
			*s->l << end;
			*s->l << code::end(block);

			*s->l << cmp(r, byteConst(0));
			*s->l << jmp(begin, ifNotEqual);
		}

		void For::toS(StrBuf *to) const {
			*to << S("for (") << testExpr << S("; ") << updateExpr << S(") ") << bodyExpr;
		}

	}
}
