#include "stdafx.h"
#include "If.h"
#include "Cast.h"
#include "Named.h"
#include "Compiler/Engine.h"
#include "Compiler/Exception.h"
#include "Compiler/Lib/MaybeTemplate.h"

namespace storm {
	namespace bs {

		IfExpr::IfExpr(SrcPos pos, Block *parent) : Block(pos, parent) {}

		void IfExpr::trueExpr(IfTrue *e) {
			trueCode = e;
		}

		void IfExpr::falseExpr(Expr *e) {
			falseCode = e;
		}


		If::If(Block *parent, Expr *cond) : IfExpr(cond->pos, parent), condition(cond) {
			if (cond->result().type().asRef(false) != Value(StormInfo<Bool>::type(engine())))
				throw TypeError(cond->pos, L"The expression must evaluate to Bool.");
		}

		ExprResult If::result() {
			if (falseCode && trueCode) {
				return common(trueCode, falseCode);
			} else {
				return ExprResult();
			}
		}

		void If::blockCode(CodeGen *state, CodeResult *r) {
			using namespace code;

			Value condType(StormInfo<Bool>::type(engine()));
			CodeResult *condResult = new (this) CodeResult(condType, state->block);
			condition->code(state, condResult);

			Label lblElse = state->to->label();
			Label lblDone = state->to->label();

			VarInfo c = condResult->location(state);
			*state->to << cmp(condResult->location(state).v, byteConst(0));
			*state->to << jmp(lblElse, ifEqual);


			Value rType = result().type();
			Expr *t = expectCastTo(trueCode, rType);
			t->code(state, r);

			if (falseCode) {
				*state->to << jmp(lblDone);
				*state->to << lblElse;

				Expr *f = expectCastTo(falseCode, rType);
				f->code(state, r);

				*state->to << lblDone;
			} else {
				*state->to << lblElse;
			}
		}

		void If::toS(StrBuf *to) const {
			*to << L"if (" << condition << L") ";
			*to << trueCode;
			if (falseCode) {
				*to << L" else ";
				*to << falseCode;
			}
		}

		/**
		 * IfWeak.
		 */

		IfWeak::IfWeak(Block *parent, WeakCast *cast) :
			IfExpr(cast->pos, parent), weakCast(cast) {}

		IfWeak::IfWeak(Block *parent, WeakCast *cast, syntax::SStr *name) :
			IfExpr(cast->pos, parent), weakCast(cast), varName(name) {}

		ExprResult IfWeak::result() {
			if (falseCode && trueCode) {
				return common(trueCode, falseCode);
			} else {
				return ExprResult();
			}
		}

		void IfWeak::toS(StrBuf *to) const {
			*to << L"if (" << weakCast << L") ";
			*to << trueCode;
			if (falseCode) {
				*to << L" else ";
				*to << falseCode;
			}
		}

		LocalVar *IfWeak::overwrite() {
			if (created)
				return created;

			Str *name;
			if (varName)
				name = varName->v;
			if (!name)
				name = weakCast->overwrite();
			if (!name)
				return null;

			created = new (this) LocalVar(name, weakCast->result(), weakCast->pos, false);
			return created;
		}

		void IfWeak::blockCode(CodeGen *state, CodeResult *r) {
			using namespace code;

			// We can create the 'created' variable here. It is fine, since it is not visible in the
			// 'false' branch anyway, and its lifetime is not going to hurt, since it does not live more
			// than a few instructions extra compared to the correct version.
			if (created)
				created->create(state);

			CodeResult *cond = new (this) CodeResult(Value(StormInfo<Bool>::type(engine())), state->block);
			weakCast->code(state, cond, created);

			Label elseLbl = state->to->label();
			Label doneLbl = state->to->label();

			*state->to << cmp(cond->location(state).v, byteConst(0));
			*state->to << jmp(elseLbl, ifEqual);

			Value rType = result().type();
			Expr *t = expectCastTo(trueCode, rType);
			t->code(state, r);

			if (falseCode) {
				*state->to << jmp(doneLbl);
			}

			*state->to << elseLbl;

			if (falseCode) {
				Expr *t = expectCastTo(falseCode, rType);
				t->code(state, r);
			}

			*state->to << doneLbl;
		}


		/**
		 * Create appropriate if statement.
		 */

		IfExpr *createIf(Block *parent, Expr *cond) {
			Value exprResult = cond->result().type().asRef(false);

			if (exprResult == Value(StormInfo<Bool>::type(parent->engine()))) {
				return new (parent) If(parent, cond);
			} else if (isMaybe(exprResult)) {
				WeakCast *cast = new (cond) WeakMaybeCast(cond);
				return new (parent) IfWeak(parent, cast);
			} else {
				throw SyntaxError(cond->pos, L"You can not use the type: " + ::toS(exprResult) + L" in an if-statement!");
			}
		}


		/**
		 * IfTrue
		 */

		IfTrue::IfTrue(SrcPos pos, Block *parent) : Block(pos, parent) {
			if (IfWeak *weak = as<IfWeak>(parent)) {
				LocalVar *override = weak->overwrite();
				if (override) {
					add(override);
				}
			}
		}

		void IfTrue::set(Expr *e) {
			expr = e;
		}

		ExprResult IfTrue::result() {
			return expr->result();
		}

		void IfTrue::blockCode(CodeGen *state, CodeResult *to) {
			expr->code(state, to);
		}

		void IfTrue::toS(StrBuf *to) const {
			*to << expr;
		}

	}
}
