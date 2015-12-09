#include "stdafx.h"
#include "BSOperator.h"
#include "BSNamed.h"
#include "BSAutocast.h"
#include "Exception.h"

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

	Bool bs::OpInfo::after(Par<OpInfo> o) {
		return o->before(this);
	}

	Bool bs::OpInfo::eq(Par<OpInfo> o) {
		return !before(o) && !after(o);
	}

	void bs::OpInfo::output(wostream &to) const {
		to << L"{name=" << name << L", left=" << (leftAssoc ? L"true" : L"false") << L"}";
	}

	bs::Expr *bs::OpInfo::meaning(Par<Block> block, Par<Expr> lhs, Par<Expr> rhs) {
		Auto<Actual> actual = CREATE(Actual, block);
		actual->add(lhs);
		actual->add(rhs);
		Auto<SStr> n = CREATE(SStr, block, name, pos);
		return namedExpr(block, n, actual);
	}

	bs::AssignOpInfo::AssignOpInfo(Par<SStr> op, Int prio, Bool leftAssoc) : OpInfo(op, prio, leftAssoc) {}

	bs::Expr *bs::AssignOpInfo::meaning(Par<Block> block, Par<Expr> lhs, Par<Expr> rhs) {
		Value l = lhs->result().type();
		Value r = rhs->result().type();

		if (l.isClass() && l.ref && castable(rhs, l.asRef(false))) {
			return CREATE(ClassAssign, block, lhs, rhs);
		} else {
			return OpInfo::meaning(block, lhs, rhs);
		}
	}


	bs::OpInfo *bs::lOperator(Par<SStr> op, Int p) {
		return CREATE(OpInfo, op, op, p, true);
	}

	bs::OpInfo *bs::rOperator(Par<SStr> op, Int p) {
		return CREATE(OpInfo, op, op, p, false);
	}

	bs::OpInfo *bs::assignOperator(Par<SStr> op, Int p) {
		return CREATE(AssignOpInfo, op, op, p, false);
	}

	bs::CombinedOperator::CombinedOperator(Par<OpInfo> info, Int prio) :
		OpInfo(steal(CREATE(SStr, engine(), info->name->v + L"=", info->pos)), prio, true), op(info) {
	}

	bs::Expr *bs::CombinedOperator::meaning(Par<Block> block, Par<Expr> lhs, Par<Expr> rhs) {
		try {
			// Is this operator overloaded?
			return OpInfo::meaning(block, lhs, rhs);
		} catch (const TypeError &) {
			// It's ok. We have a backup plan!
		} catch (const SyntaxError &) {
			// It's ok. We have a backup plan!
		}

		try {
			// Create: 'lhs = lhs <op> rhs'
			Auto<SStr> eqOp = CREATE(SStr, this, L"=", pos);
			Auto<OpInfo> assign = CREATE(AssignOpInfo, this, eqOp, 100, true);

			Auto<Expr> middle = op->meaning(block, lhs, rhs);
			return assign->meaning(block, lhs, middle);
		} catch (...) {
			TODO(L"Better error message when using combined operators.");
			throw;
		}
	}

	bs::Operator::Operator(Par<Block> block, Par<Expr> lhs, Par<OpInfo> op, Par<Expr> rhs)
		: block(block.borrow()), lhs(lhs), op(op), rhs(rhs), fnCall(null) {}

	// Left should be true if 'other' is 'our' left child.
	static bool after(Par<bs::OpInfo> our, Par<bs::OpInfo> other, bool left) {
		if (our->after(other))
			return true;
		if (our->leftAssoc == left && our->eq(other))
			return true;
		return false;
	}

	bs::Operator *bs::Operator::prioritize() {
		Auto<Operator> l = lhs.as<Operator>();
		Auto<Operator> r = rhs.as<Operator>();
		Auto<Operator> me = capture(this);

		invalidate();

		if (l && r) {
			// In:  (1 l 2) me (3 r 4)
			// Out1 (1 l (2 me 3)) r 4 if l after me && l before r && me before r
			// Out2 1 l ((2 me 3) r 4) if l after me && l after r && me before r
			// Out3 1 l (2 me (3 r 4)) if l after me && me after r
			// Out4 ((1 l 2) me 3) r 4 if l before me && me before r
			// Out5 (1 l 2) me (3 r 4) if l before me && me after r

			if (after(op, l->op, true)) {
				// Equal to: X me (3 r 4)
				// Out4 X me (3 r 4) if me before r
				// Out5 (X me 3) r 4 if me after r

				// Ignore l, use the r swap below...
				l = null;
			} else if (after(op, r->op, false)) {
				// Equal to: (1 l 2) me X
				// Out3 1 l (2 me X) if l after me
				// Out5 (1 l 2) me X if l before me

				// Ignore r, use the l swap below.
				r = null;
			} else {
				// Out1 (1 l (2 me 3)) r 4 if l before r
				// Out2 1 l ((2 me 3) r 4) if l after r
				// We can set: X = 2 me 3 and get:
				// Out1 (1 l X) r 4 if l before r
				// Out2 1 l (X r 4) if l after r

				lhs = l->rhs;
				rhs = r->lhs;

				if (after(l->op, r->op, false)) {
					// Out2
					r->lhs = me->prioritize();
					l->rhs = r;

					return l.ret();
				} else {
					// Out1
					l->rhs = me->prioritize();
					r->lhs = l;

					return r.ret();
				}
			}
		}

		if (l) {
			// In:  (1 l 2) me 3
			// Out1 1 l (2 me 3) if me before l
			// Out2 (1 l 2) me 3 if me after l

			// Should the other one run first?
			if (after(op, l->op, true))
				return me.ret();

			// Switch.
			lhs = l->rhs;
			l->rhs = me->prioritize();

			return l.ret();
		} else if (r) {
			// In:  1 me (2 r 3)
			// Out1 1 me (2 r 3) if me after r
			// Out2 (1 me 2) r 3 if me before r

			// Should the other one run first?
			if (after(op, r->op, false))
				return me.ret();

			// Switch ourselves with the other one.
			rhs = r->lhs;
			r->lhs = me->prioritize();

			return r.ret();
		} else {
			// We're a leaf operator.
			return me.ret();
		}
	}

	void bs::Operator::invalidate() {
		fnCall = null;
	}

	bs::Expr *bs::Operator::meaning() {
		if (!fnCall)
			fnCall = op->meaning(block, lhs, rhs);
		return fnCall.ret();
	}

	ExprResult bs::Operator::result() {
		Auto<Expr> m = meaning();
		return m->result();
	}

	void bs::Operator::code(Par<CodeGen> s, Par<CodeResult> r) {
		Auto<Expr> m = meaning();
		return m->code(s, r);
	}

	void bs::Operator::output(wostream &to) const {
		if (Operator *l = as<Operator>(lhs.borrow()))
			to << L"<" << lhs << L">";
		else
			to << lhs;
		to << L" " << op->name << L" ";
		if (Operator *r = as<Operator>(rhs.borrow()))
			to << L"<" << rhs << L">";
		else
			to << rhs;
	}

	bs::Operator *bs::mkOperator(Par<Block> block, Par<Expr> lhs, Par<OpInfo> op, Par<Expr> rhs) {
		Auto<Operator> o = CREATE(Operator, op, block, lhs, op, rhs);
		return o->prioritize();
	}

	/**
	 * Parens.
	 */

	bs::ParenExpr::ParenExpr(Par<Expr> wrap) : wrap(wrap) {}

	ExprResult bs::ParenExpr::result() {
		return wrap->result();
	}

	void bs::ParenExpr::code(Par<CodeGen> s, Par<CodeResult> r) {
		wrap->code(s, r);
	}

	void bs::ParenExpr::output(wostream &to) const {
		to << L"(" << wrap << L")";
	}

	/**
	 * Create standard operators.
	 */

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
