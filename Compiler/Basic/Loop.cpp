#include "stdafx.h"
#include "Loop.h"
#include "Compiler/Exception.h"

namespace storm {
	namespace bs {

		Loop::Loop(SrcPos pos, Block *parent) : Block(pos, parent) {}

		void Loop::cond(Condition *cond) {
			condition = cond;
		}

		Condition *Loop::cond() {
			if (!condition)
				throw new (this) RuntimeError(S("Must call 'cond' before creating a while body!"));
			return condition;
		}

		void Loop::condExpr(Expr *e) {
			cond(new (this) BoolCondition(e));
		}

		void Loop::doBody(Expr *e) {
			doExpr = e;
			if (Block *b = as<Block>(e))
				liftVars(b);
		}

		void Loop::whileBody(CondSuccess *e) {
			whileExpr = e;
		}

		CondSuccess *Loop::createWhileBody() {
			if (!condition)
				throw new (this) RuntimeError(S("Must call 'cond' before creating a while body!"));
			return new (this) CondSuccess(pos, this, condition);
		}

		ExprResult Loop::result() {
			if (condition) {
				// No reliable value, the last expression in the do part could be used.
				return ExprResult();
			} else {
				// If we don't have a condition, we never return.
				return noReturn();
			}
		}

		void Loop::code(CodeGen *s, CodeResult *r) {
			using namespace code;

			// Outer state where we store our control variable!
			CodeGen *outer = s->child();
			*s->l << begin(outer->block);

			// Inner state, which represents the actual block we're in.
			CodeGen *inner = outer->child();

			// Initialize variables in the scope.
			initVariables(inner);


			code(outer, inner, r);

			*s->l << end(outer->block);

			// May be delayed...
			if (r->needed())
				r->location(s).created(s);
		}

		void Loop::code(CodeGen *outerState, CodeGen *innerState, CodeResult *r) {
			using namespace code;

			Label before = innerState->l->label();
			CodeResult *condResult = null;

			*innerState->l << before;
			*innerState->l << begin(innerState->block);

			if (doExpr) {
				CodeResult *doResult = new (this) CodeResult();
				doExpr->code(innerState, doResult);
			}

			if (condition) {
				condResult = new (this) CodeResult(Value(StormInfo<Bool>::type(engine())), outerState->block);
				condition->code(innerState, condResult);
			}

			if (whileExpr) {
				Label after = innerState->l->label();

				if (condResult) {
					code::Var c = condResult->location(innerState).v;
					*innerState->l << cmp(c, byteConst(0));
					*innerState->l << jmp(after, ifEqual);
				}

				CodeResult *whileResult = new (this) CodeResult();
				whileExpr->code(innerState, whileResult);

				*innerState->l << after;
			}

			*innerState->l << end(innerState->block);

			if (condResult) {
				code::Var c = condResult->location(outerState).v;
				*innerState->l << cmp(c, byteConst(0));
				*innerState->l << jmp(before, ifNotEqual);
			} else {
				*innerState->l << jmp(before);
			}
		}

		void Loop::toS(StrBuf *to) const {
			if (doExpr) {
				*to << S("do ") << doExpr;
			}

			if (condition) {
				*to << S("while (") << condition << S(") ");
			}

			if (whileExpr) {
				*to << whileExpr;
			}
		}

	}
}
