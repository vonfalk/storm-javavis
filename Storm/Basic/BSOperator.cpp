#include "stdafx.h"
#include "BSOperator.h"
#include "BSNamed.h"

namespace storm {

	bs::OpInfo::OpInfo(Par<SStr> op, Int prio, Bool leftAssoc)
		: priority(prio), leftAssoc(leftAssoc), name(op->v), pos(op->pos) {}

	Bool bs::OpInfo::before(Par<OpInfo> o) {
		if (priority != o->priority)
			return priority > o->priority;

		// Left associative operators are stronger.
		if (leftAssoc && !o->leftAssoc)
			return true;

		return false;
	}

	bs::OpInfo *bs::lOperator(Par<SStr> op, Int p) {
		return CREATE(OpInfo, op, op, p, true);
	}

	bs::OpInfo *bs::rOperator(Par<SStr> op, Int p) {
		return CREATE(OpInfo, op, op, p, false);
	}

	bs::Operator::Operator(Par<Block> block, Par<Expr> lhs, Par<OpInfo> op, Par<Expr> rhs)
		: block(block.borrow()), lhs(lhs), op(op), rhs(rhs) {}

	void bs::Operator::output(wostream &to) const {
		to << L"(" << lhs << L" " << op->name << L" " << rhs << L")";
	}


	/**
	 * Create standard operators.
	 */

	bs::Expr *bs::assignExpr(Par<Block> block, Par<Expr> lhs, Par<SStr> m, Par<Expr> rhs) {
		Value r = lhs->result();
		if (r.type) {
			if (r.type->flags & typeClass) {
				return CREATE(ClassAssign, block, lhs, rhs);
			}
		}

		Auto<Actual> actual = CREATE(Actual, block);
		actual->add(lhs);
		actual->add(rhs);
		return namedExpr(block, m, actual);
	}


	bs::Expr *STORM_FN bs::accessExpr(Par<Block> block, Par<Expr> lhs, Par<Expr> par) {
		Auto<Actual> actual = CREATE(Actual, block);
		actual->add(lhs);
		actual->add(par);
		Auto<SStr> m = CREATE(SStr, block, L"[]");
		return namedExpr(block, m, actual);
	}

	bs::Expr *STORM_FN bs::prefixOperator(Par<Block> block, Par<SStr> o, Par<Expr> expr) {
		Auto<Actual> actual = CREATE(Actual, block);
		actual->add(expr);
		o->v->v = o->v->v + L"*";
		return namedExpr(block, o, actual);
	}

	bs::Expr *STORM_FN bs::postfixOperator(Par<Block> block, Par<SStr> o, Par<Expr> expr) {
		Auto<Actual> actual = CREATE(Actual, block);
		actual->add(expr);
		o->v->v = L"*" + o->v->v;
		return namedExpr(block, o, actual);
	}


}
