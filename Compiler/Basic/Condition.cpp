#include "stdafx.h"
#include "Condition.h"
#include "Compiler/Exception.h"

namespace storm {
	namespace bs {

		/**
		 * Condition.
		 */

		Condition::Condition() {}

		SrcPos Condition::pos() {
			return SrcPos();
		}

		MAYBE(LocalVar *) Condition::result() {
			return null;
		}

		void Condition::code(CodeGen *state, CodeResult *ok) {
			throw AbstractFnCalled(L"Condition::code");
		}


		/**
		 * Boolean condition.
		 */

		BoolCondition::BoolCondition(Expr *expr) : expr(expr) {
			if (expr->result().type().asRef(false) != Value(StormInfo<Bool>::type(engine())))
				throw TypeError(expr->pos, L"The expression must evaluate to Bool.");
		}

		SrcPos BoolCondition::pos() {
			return expr->pos;
		}

		MAYBE(LocalVar *) BoolCondition::result() {
			// We never create anything!
			return null;
		}

		void BoolCondition::code(CodeGen *state, CodeResult *ok) {
			expr->code(state, ok);
		}

		void BoolCondition::toS(StrBuf *to) const {
			*to << expr;
		}


		/**
		 * Weak condition.
		 */

		WeakCondition::WeakCondition(WeakCast *cast) : cast(cast), varName(null) {}

		WeakCondition::WeakCondition(syntax::SStr *name, WeakCast *cast) : cast(cast), varName(name) {}

		SrcPos WeakCondition::pos() {
			return cast->pos;
		}

		MAYBE(LocalVar *) WeakCondition::result() {
			if (created)
				return created;

			Str *name = null;
			SrcPos pos;
			if (varName) {
				name = varName->v;
				pos = varName->pos;
			} else {
				name = cast->overwrite();
				pos = cast->pos;
			}

			if (!name)
				return null;

			created = new (this) LocalVar(name, cast->result(), pos, false);
			return created;
		}

		void WeakCondition::code(CodeGen *state, CodeResult *ok) {
			LocalVar *var = result();
			if (var)
				var->create(state);

			cast->code(state, ok, var);
		}

		void WeakCondition::toS(StrBuf *to) const {
			if (varName)
				*to << varName->v << S(" = ");
			*to << cast;
		}


		// Create suitable conditions.
		Condition *STORM_FN createCondition(Expr *expr) {
			Value result = expr->result().type().asRef(false);

			if (result == Value(StormInfo<Bool>::type(expr->engine()))) {
				return new (expr) BoolCondition(expr);
			} else if (isMaybe(result)) {
				WeakCast *cast = new (expr) WeakMaybeCast(expr);
				return new (expr) WeakCondition(cast);
			} else {
				throw SyntaxError(expr->pos, L"You can not use the type " + ::toS(result) + L" as a condition.");
			}
		}


		/**
		 * CondSuccess.
		 */

		CondSuccess::CondSuccess(SrcPos pos, Block *parent, Condition *cond) : Block(pos, parent) {
			if (LocalVar *var = cond->result())
				add(var);
		}

		void CondSuccess::set(Expr *e) {
			if (expr)
				throw RuntimeError(L"Cannot call CondSuccess::set multiple times!");
			expr = e;
		}

		void CondSuccess::replace(Expr *e) {
			expr = e;
		}

		ExprResult CondSuccess::result() {
			if (expr)
				return expr->result();
			else
				return ExprResult();
		}

		void CondSuccess::blockCode(CodeGen *state, CodeResult *to) {
			if (expr)
				expr->code(state, to);
		}

		Int CondSuccess::castPenalty(Value to) {
			if (expr)
				return expr->castPenalty(to);
			else
				return -1;
		}

		void CondSuccess::toS(StrBuf *to) const {
			if (expr)
				*to << expr;
		}

	}
}
