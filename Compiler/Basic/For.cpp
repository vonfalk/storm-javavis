#include "stdafx.h"
#include "For.h"
#include "Cast.h"

namespace storm {
	namespace bs {

		For::For(SrcPos pos, Block *parent) : Breakable(pos, parent) {}

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

			this->block = block;
			Label begin = s->l->label();
			Label end = s->l->label();
			brk = end;
			cont = s->l->label();
			CodeGen *subState = s->child(block);

			*s->l << begin;
			*s->l << code::begin(block);

			// Put this outside the current block, so we can use it later.
			CodeResult *testResult = new (this) CodeResult(Value(StormInfo<Bool>::type(engine())), block);
			testExpr->code(subState, testResult);
			code::Var r = testResult->location(subState);
			*s->l << cmp(r, byteConst(0));
			*s->l << jmp(end, ifEqual);

			CodeResult *bodyResult = CREATE(CodeResult, this);
			bodyExpr->code(subState, bodyResult);

			*s->l << cont;
			CodeResult *updateResult = CREATE(CodeResult, this);
			updateExpr->code(subState, updateResult);

			// Jump back.
			*s->l << jmpBlock(begin, s->block);

			*s->l << end;
			*s->l << code::end(block);
		}

		void For::willBreak() {}

		void For::willContinue() {}

		Breakable::To For::breakTo() {
			return Breakable::To(brk, block);
		}

		Breakable::To For::continueTo() {
			return Breakable::To(cont, block);
		}

		void For::toS(StrBuf *to) const {
			*to << S("for (") << testExpr << S("; ") << updateExpr << S(") ") << bodyExpr;
		}

	}
}
