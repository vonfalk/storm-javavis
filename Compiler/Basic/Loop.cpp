#include "stdafx.h"
#include "Loop.h"

namespace storm {
	namespace bs {

		Loop::Loop(SrcPos pos, Block *parent) : Block(pos, parent) {}

		void Loop::cond(Expr *e) {
			condExpr = e;
		}

		void Loop::doBody(Expr *e) {
			doExpr = e;
			if (Block *b = as<Block>(e))
				liftVars(b);
		}

		void Loop::whileBody(Expr *e) {
			whileExpr = e;
		}

		ExprResult Loop::result() {
			// No reliable value, the last expression in the do part could be used.
			return ExprResult();
		}

		void Loop::blockCode(CodeGen *s, CodeResult *r, const code::Block &block) {
			using namespace code;

			Label before = s->l->label();
			CodeGen *subState = s->child(block);
			CodeResult *condResult = null;

			// Begin code generation!
			*s->l << before;
			*s->l << begin(block);

			if (doExpr) {
				CodeResult *doResult = CREATE(CodeResult, this);
				doExpr->code(subState, doResult);
			}

			if (condExpr) {
				// Place a control variable outside of this scope. This should maybe be in a separate scope.
				condResult = new (this) CodeResult(Value(StormInfo<Bool>::type(engine())), s->block);
				condExpr->code(subState, condResult);
			}

			if (whileExpr) {
				Label after = s->l->label();

				if (condExpr) {
					code::Var c = condResult->location(s).v;
					*s->l << cmp(c, byteConst(0));
					*s->l << jmp(after, ifEqual);
				}

				CodeResult *whileResult = CREATE(CodeResult, this);
				whileExpr->code(subState, whileResult);

				*s->l << after;
				*s->l << end(block);
			}

			if (condExpr) {
				code::Var c = condResult->location(s).v;
				*s->l << cmp(c, byteConst(0));
			}
			*s->l << jmp(before, ifNotEqual);
		}

		void Loop::toS(StrBuf *to) const {
			if (doExpr) {
				*to << L"do " << doExpr;
			}

			if (condExpr) {
				*to << L"while (" << condExpr << L") ";
			}

			if (whileExpr) {
				*to << whileExpr;
			}
		}

	}
}
