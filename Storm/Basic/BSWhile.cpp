#include "stdafx.h"
#include "BSWhile.h"
#include "Exception.h"

namespace storm {

	bs::While::While(Par<Block> parent) : Block(parent) {}

	void bs::While::cond(Par<Expr> e) {
		e->result().mustStore(Value::stdBool(engine()), e->pos);
		condExpr = e;
	}

	void bs::While::body(Par<Expr> e) {
		bodyExpr = e;
	}

	Value bs::While::result() {
		// No reliable value in a while loop, since it may be executed zero times.
		return Value();
	}

	void bs::While::blockCode(const GenState &s, GenResult &r, const code::Block &block) {
		using namespace code;

		Label begin = s.to.label();
		Label end = s.to.label();
		GenState subState = { s.to, s.data, s.frame, block };

		s.to << begin;
		s.to << code::begin(block);

		GenResult condResult(Value::stdBool(engine()), block);
		condExpr->code(subState, condResult);
		s.to << cmp(condResult.location(subState), byteConst(0));
		s.to << jmp(end, ifEqual);

		GenResult bodyResult;
		bodyExpr->code(subState, bodyResult);

		s.to << code::end(block);
		s.to << jmp(begin);
		s.to << end;
	}

}
