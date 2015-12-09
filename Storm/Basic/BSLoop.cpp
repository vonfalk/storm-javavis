#include "stdafx.h"
#include "BSLoop.h"

namespace storm {

	bs::Loop::Loop(Par<Block> parent) : Block(parent) {}

	void bs::Loop::cond(Par<Expr> e) {
		condExpr = e;
	}

	void bs::Loop::doBody(Par<Expr> e) {
		doExpr = e;
		if (Par<Block> b = e.as<Block>())
			liftVars(b);
	}

	void bs::Loop::whileBody(Par<Expr> e) {
		whileExpr = e;
	}

	ExprResult bs::Loop::result() {
		// No reliable value, the last expression in the do part could be used.
		return ExprResult();
	}

	void bs::Loop::blockCode(Par<CodeGen> s, Par<CodeResult> r, const code::Block &block) {
		using namespace code;

		Label before = s->to.label();
		Auto<CodeGen> subState = s->child(block);
		Auto<CodeResult> condResult;

		// Begin code generation!
		s->to << before;
		s->to << begin(block);

		if (doExpr) {
			Auto<CodeResult> doResult = CREATE(CodeResult, this);
			doExpr->code(subState, doResult);
		}

		if (condExpr) {
			// Place a control variable outside of this scope. This should maybe be in a separate scope.
			condResult = CREATE(CodeResult, this, Value::stdBool(engine()), s->block);
			condExpr->code(subState, condResult);
		}

		if (whileExpr) {
			Label after = s->to.label();

			if (condExpr) {
				Variable c = condResult->location(s).var();
				s->to << cmp(c, byteConst(0));
				s->to << jmp(after, ifEqual);
			}

			Auto<CodeResult> whileResult = CREATE(CodeResult, this);
			whileExpr->code(subState, whileResult);

			s->to << after;
			s->to << end(block);
		}

		if (condExpr) {
			Variable c = condResult->location(s).var();
			s->to << cmp(c, byteConst(0));
		}
		s->to << jmp(before, ifNotEqual);
	}

	void bs::Loop::output(wostream &to) const {
		if (doExpr) {
			to << "do " << doExpr;
		}

		if (condExpr) {
			to << "while (" << condExpr << ") ";
		}

		if (whileExpr) {
			to << whileExpr;
		}
	}

}
