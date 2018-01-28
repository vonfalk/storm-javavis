#include "stdafx.h"
#include "Operator.h"
#include "Named.h"
#include "Cast.h"
#include "Compiler/Type.h"
#include "Compiler/Exception.h"

namespace storm {
	namespace bs {

		OpInfo::OpInfo(syntax::SStr *op, Int prio, Bool leftAssoc)
			: priority(prio), leftAssoc(leftAssoc), name(op->v), pos(op->pos) {}

		Bool OpInfo::before(OpInfo *o) {
			if (priority != o->priority)
				return priority > o->priority;

			// Left associative operators are stronger.
			if (leftAssoc && !o->leftAssoc)
				return true;

			return false;
		}

		Bool OpInfo::after(OpInfo *o) {
			return o->before(this);
		}

		Bool OpInfo::eq(OpInfo *o) {
			return !before(o) && !after(o);
		}

		void OpInfo::toS(StrBuf *to) const {
			*to << L"{name=" << name << L", left=" << (leftAssoc ? L"true" : L"false") << L"}";
		}

		Expr *OpInfo::meaning(Block *block, Expr *lhs, Expr *rhs) {
			Expr *fn = find(block, name, lhs, rhs);
			if (!fn)
				throw SyntaxError(pos, L"Can not find an implementation of the operator " +
								::toS(name) + L" for " + ::toS(lhs->result().type()) + L", " +
								::toS(rhs->result().type()) + L".");

			return fn;
		}

		Expr *OpInfo::find(Block *block, Str *name, Expr *lhs, Expr *rhs) {
			Actuals *actuals = new (this) Actuals();
			actuals->add(lhs);
			actuals->add(rhs);

			BSNamePart *part = new (this) BSNamePart(name, pos, actuals);
			SimpleName *sName = new (this) SimpleName(part);

			Named *found = block->scope.find(sName);
			if (!found) {
				ExprResult r = rhs->result();
				if (r.any()) {
					// Try to find the function inside 'rhs' as well. Auto-casting might make that work.
					found = r.type().type->find(part, block->scope);
				}
			}

			Function *fn = as<Function>(found);
			if (!fn)
				return null;

			return new (this) FnCall(pos, block->scope, fn, actuals);
		}

		OpInfo *lOperator(syntax::SStr *op, Int p) {
			return new (op) OpInfo(op, p, true);
		}

		OpInfo *rOperator(syntax::SStr *op, Int p) {
			return new (op) OpInfo(op, p, false);
		}


		Operator::Operator(Block *block, Expr *lhs, OpInfo *op, Expr *rhs)
			: Expr(op->pos), block(block), lhs(lhs), op(op), rhs(rhs), fnCall(null) {}

		// Left should be true if 'other' is 'our' left child.
		static bool after(bs::OpInfo *our, OpInfo *other, bool left) {
			if (our->after(other))
				return true;
			if (our->leftAssoc == left && our->eq(other))
				return true;
			return false;
		}

		Operator *Operator::prioritize() {
			Operator *l = as<Operator>(lhs);
			Operator *r = as<Operator>(rhs);
			Operator *me = this;

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

						return l;
					} else {
						// Out1
						l->rhs = me->prioritize();
						r->lhs = l;

						return r;
					}
				}
			}

			if (l) {
				// In:  (1 l 2) me 3
				// Out1 1 l (2 me 3) if me before l
				// Out2 (1 l 2) me 3 if me after l

				// Should the other one run first?
				if (after(op, l->op, true))
					return me;

				// Switch.
				lhs = l->rhs;
				l->rhs = me->prioritize();

				return l;
			} else if (r) {
				// In:  1 me (2 r 3)
				// Out1 1 me (2 r 3) if me after r
				// Out2 (1 me 2) r 3 if me before r

				// Should the other one run first?
				if (after(op, r->op, false))
					return me;

				// Switch ourselves with the other one.
				rhs = r->lhs;
				r->lhs = me->prioritize();

				return r;
			} else {
				// We're a leaf operator.
				return me;
			}
		}

		void Operator::invalidate() {
			fnCall = null;
		}

		Expr *Operator::meaning() {
			if (!fnCall)
				fnCall = op->meaning(block, lhs, rhs);
			return fnCall;
		}

		ExprResult Operator::result() {
			Expr *m = meaning();
			return m->result();
		}

		void Operator::code(CodeGen *s, CodeResult *r) {
			Expr *m = meaning();
			return m->code(s, r);
		}

		void Operator::toS(StrBuf *to) const {
			if (Operator *l = as<Operator>(lhs))
				*to << S("<") << l << S(">");
			else
				*to << lhs;
			*to << S(" ") << op->name << S(" ");
			if (Operator *r = as<Operator>(rhs))
				*to << S("<") << r << S(">");
			else
				*to << rhs;
		}

		Operator *mkOperator(Block *block, Expr *lhs, OpInfo *op, Expr *rhs) {
			Operator *o = new (op) Operator(block, lhs, op, rhs);
			return o->prioritize();
		}

		/**
		 * Parens.
		 */

		ParenExpr::ParenExpr(Expr *wrap) : Expr(wrap->pos), wrap(wrap) {}

		ExprResult ParenExpr::result() {
			return wrap->result();
		}

		void ParenExpr::code(CodeGen *s, CodeResult *r) {
			wrap->code(s, r);
		}

		void ParenExpr::toS(StrBuf *to) const {
			*to << S("(") << wrap << S(")");
		}

		/**
		 * Create standard operators.
		 */

		Expr *STORM_FN accessExpr(Block *block, Expr *lhs, Expr *par) {
			Actuals *actual = new (block) Actuals();
			actual->add(lhs);
			actual->add(par);
			syntax::SStr *m = new (block) syntax::SStr(S("[]"));
			return namedExpr(block, m, actual);
		}

		Expr *STORM_FN prefixOperator(Block *block, syntax::SStr *o, Expr *expr) {
			Actuals *actual = new (block) Actuals();
			actual->add(expr);
			syntax::SStr *altered = new (o) syntax::SStr(*o->v + new (o) Str(S("*")), o->pos);
			return namedExpr(block, altered, actual);
		}

		Expr *STORM_FN postfixOperator(Block *block, syntax::SStr *o, Expr *expr) {
			Actuals *actual = new (block) Actuals();
			actual->add(expr);
			syntax::SStr *altered = new (o) syntax::SStr(*new (o) Str(S("*")) + o->v, o->pos);
			return namedExpr(block, altered, actual);
		}

		Expr *STORM_FN prePostOperator(Block *block, syntax::SStr *o, Expr *expr) {
			Actuals *actual = new (block) Actuals();
			actual->add(expr);
			return namedExpr(block, o, actual);
		}

	}
}
