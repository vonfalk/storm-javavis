#include "stdafx.h"
#include "BSFor.h"
#include "BSAutocast.h"

namespace storm {

	bs::For::For(Par<Block> parent) : Block(parent) {}

	void bs::For::test(Par<Expr> e) {
		testExpr = expectCastTo(e, Value::stdBool(engine()));
	}

	void bs::For::update(Par<Expr> e) {
		updateExpr = e;
	}

	void bs::For::body(Par<Expr> e) {
		bodyExpr = e;
	}

	Value bs::For::result() {
		return Value();
	}

	void bs::For::blockCode(Par<CodeGen> s, Par<CodeResult> to, const code::Block &block) {
		using namespace code;

		Label begin = s->to.label();
		Label end = s->to.label();
		Auto<CodeGen> subState = s->child(block);

		s->to << begin;
		s->to << code::begin(block);

		// Put this outside the current block, so we can use it later.
		Auto<CodeResult> testResult = CREATE(CodeResult, this, Value::stdBool(engine()), s->block.v);
		testExpr->code(subState, testResult);
		Variable r = testResult->location(subState).var();
		s->to << cmp(r, byteConst(0));
		s->to << jmp(end, ifEqual);

		Part before = s->frame.last(block);

		Auto<CodeResult> bodyResult = CREATE(CodeResult, this);
		bodyExpr->code(subState, bodyResult);

		Auto<CodeResult> updateResult = CREATE(CodeResult, this);
		updateExpr->code(subState, updateResult);

		// Make sure to end all parts that may have been created so far.
		Part after = s->frame.next(before);
		if (after != Part::invalid)
			s->to << code::end(after);

		// We may not skip the 'end'.
		s->to << end;
		s->to << code::end(block);

		s->to << cmp(r, byteConst(0));
		s->to << jmp(begin, ifNotEqual);
	}

	void bs::For::output(wostream &to) const {
		to << L"for (" << testExpr << L"; " << updateExpr << L") " << bodyExpr;
	}
}
