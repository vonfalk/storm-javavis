#include "stdafx.h"
#include "BSIf.h"
#include "Exception.h"

namespace storm {

	bs::If::If(Auto<Block> parent) : Block(parent) {}

	void bs::If::cond(Auto<Expr> e) {
		if (e->result() != Value(boolType(engine())))
			throw TypeError(e->pos, L"The expression must evaluate to Bool.");
		condition = e;
	}

	void bs::If::trueExpr(Auto<Expr> e) {
		trueCode = e;
	}

	void bs::If::falseExpr(Auto<Expr> e) {
		falseCode = e;
	}

	Value bs::If::result() {
		if (falseCode && trueCode) {
			Value t = trueCode->result();
			Value f = falseCode->result();
			return common(t, f);
		} else {
			return Value();
		}
	}

	void bs::If::blockCode(const GenState &state, GenResult &r) {
		using namespace code;

		GenResult condResult(state.block);
		condition->code(state, condResult);

		Label lblElse = state.to.label();
		Label lblDone = state.to.label();

		state.to << cmp(condResult.location(state, Value(boolType(engine()))), byteConst(0));
		state.to << jmp(lblElse, ifEqual);

		trueCode->code(state, r);

		if (falseCode) {
			state.to << jmp(lblDone);
			state.to << lblElse;

			falseCode->code(state, r);

			state.to << lblDone;
		} else {
			state.to << lblElse;
		}
	}

}
