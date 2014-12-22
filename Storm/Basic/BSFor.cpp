#include "stdafx.h"
#include "BSFor.h"

namespace storm {

	bs::For::For(Auto<Block> parent) : Block(parent) {}

	void bs::For::start(Auto<Expr> e) {
		startExpr = e;
	}

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

	void bs::For::blockCode(const GenState &s, GenResult &to) {
		using namespace code;

		Label begin = s.to.label();
		Label end = s.to.label();

		TODO(L"Fix a new scope for the second and third part of the for loop.");

		GenResult startResult;
		startExpr->code(s, startResult);

		s.to << begin;

		GenResult testResult(s.block);
		testExpr->code(s, testResult);
		s.to << cmp(testResult.location(s, Value::stdBool(engine())), byteConst(0));
		s.to << jmp(end, ifEqual);

		GenResult bodyResult;
		bodyExpr->code(s, bodyResult);

		GenResult updateResult;
		updateExpr->code(s, updateResult);

		s.to << jmp(begin);
		s.to << end;
	}

}
