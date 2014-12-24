#include "stdafx.h"
#include "BSFor.h"

namespace storm {

	bs::For::For(Auto<Block> parent) : Block(parent) {}

	void bs::For::test(Auto<Expr> e) {
		e->result().mustStore(Value::stdBool(engine()), e->pos);
		testExpr = e;
	}

	void bs::For::update(Auto<Expr> e) {
		updateExpr = e;
	}

	void bs::For::body(Auto<Expr> e) {
		bodyExpr = e;
	}

	Value bs::For::result() {
		return Value();
	}

	void bs::For::blockCode(const GenState &s, GenResult &to, const code::Block &block) {
		using namespace code;

		Label begin = s.to.label();
		Label end = s.to.label();
		GenState subState = { s.to, s.frame, block };

		s.to << begin;
		s.to << code::begin(block);

		GenResult testResult(Value::stdBool(engine()), block);
		testExpr->code(subState, testResult);
		s.to << cmp(testResult.location(subState), byteConst(0));
		s.to << jmp(end, ifEqual);

		GenResult bodyResult;
		bodyExpr->code(subState, bodyResult);

		GenResult updateResult;
		updateExpr->code(subState, updateResult);

		s.to << code::end(block);
		s.to << jmp(begin);
		s.to << end;
	}

}
