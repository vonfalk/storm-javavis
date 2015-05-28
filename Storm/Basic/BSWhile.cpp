#include "stdafx.h"
#include "BSWhile.h"
#include "BSAutocast.h"

namespace storm {

	bs::While::While(Par<Block> parent) : Block(parent) {}

	void bs::While::cond(Par<Expr> e) {
		condExpr = expectCastTo(e, Value::stdBool(engine()));
	}

	void bs::While::body(Par<Expr> e) {
		bodyExpr = e;
	}

	Value bs::While::result() {
		// No reliable value in a while loop, since it may be executed zero times.
		return Value();
	}

	void bs::While::blockCode(Par<CodeGen> s, Par<CodeResult> r, const code::Block &block) {
		using namespace code;

		Label begin = s->to.label();
		Label end = s->to.label();
		Auto<CodeGen> subState = s->child(block);

		s->to << begin;
		s->to << code::begin(block);

		// Place this in the parent block.
		Auto<CodeResult> condResult = CREATE(CodeResult, this, Value::stdBool(engine()), s->block);
		condExpr->code(subState, condResult);
		Variable c = condResult->location(subState).var();
		s->to << cmp(c, byteConst(0));
		s->to << jmp(end, ifEqual);

		Part before = s->frame.last(block);

		Auto<CodeResult> bodyResult = CREATE(CodeResult, this);
		bodyExpr->code(subState, bodyResult);

		Part after = s->frame.next(before);
		if (after != Part::invalid)
			s->to << code::end(after);

		// We may not jump across the end below.
		s->to << end;
		s->to << code::end(block);
		s->to << cmp(c, byteConst(0));
		s->to << jmp(begin, ifNotEqual);
	}

	void bs::While::output(wostream &to) const {
		to << L"while (" << condExpr << L")" << bodyExpr;
	}

}
