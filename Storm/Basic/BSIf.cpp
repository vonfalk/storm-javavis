#include "stdafx.h"
#include "BSIf.h"
#include "BSAutocast.h"
#include "Exception.h"
#include "BSNamed.h"
#include "Lib/Maybe.h"
#include "Engine.h"

namespace storm {

	bs::If::If(Par<Block> parent, Par<Expr> cond) : Block(parent), condition(cond) {
		if (cond->result().type().asRef(false) != Value(boolType(engine())))
			throw TypeError(cond->pos, L"The expression must evaluate to Bool.");
	}

	void bs::If::trueExpr(Par<Expr> e) {
		trueCode = e;
	}

	void bs::If::falseExpr(Par<Expr> e) {
		falseCode = e;
	}

	ExprResult bs::If::result() {
		if (falseCode && trueCode) {
			return common(trueCode, falseCode);
		} else {
			return ExprResult();
		}
	}

	void bs::If::blockCode(Par<CodeGen> state, Par<CodeResult> r) {
		using namespace code;

		Value condType = boolType(engine());
		Auto<CodeResult> condResult = CREATE(CodeResult, this, condType, state->block);
		condition->code(state, condResult);

		Label lblElse = state->to.label();
		Label lblDone = state->to.label();

		VarInfo c = condResult->location(state);
		state->to << cmp(condResult->location(state).var(), byteConst(0));
		state->to << jmp(lblElse, ifEqual);


		Value rType = result().type();
		Auto<Expr> t = expectCastTo(trueCode, rType);
		t->code(state, r);

		if (falseCode) {
			state->to << jmp(lblDone);
			state->to << lblElse;

			Auto<Expr> f = expectCastTo(falseCode, rType);
			f->code(state, r);

			state->to << lblDone;
		} else {
			state->to << lblElse;
		}
	}

	void bs::If::output(wostream &to) const {
		to << L"if (" << condition << L") ";
		to << trueCode;
		if (falseCode) {
			to << L" else ";
			to << falseCode;
		}
	}

	/**
	 * IfWeak.
	 */

	bs::IfWeak::IfWeak(Par<Block> parent, Par<WeakCast> cast) :
		Block(parent), weakCast(cast) {}

	bs::IfWeak::IfWeak(Par<Block> parent, Par<WeakCast> cast, Par<SStr> name) :
		Block(parent), weakCast(cast), varName(name) {}

	void bs::IfWeak::trueExpr(Par<IfTrue> e) {
		trueCode = e;
	}

	void bs::IfWeak::falseExpr(Par<Expr> e) {
		falseCode = e;
	}

	ExprResult bs::IfWeak::result() {
		if (falseCode && trueCode) {
			return common(trueCode, falseCode);
		} else {
			return ExprResult();
		}
	}

	void bs::IfWeak::output(wostream &to) const {
		to << L"if (" << weakCast << L") ";
		to << trueCode;
		if (falseCode) {
			to << L" else ";
			to << falseCode;
		}
	}

	bs::LocalVar *bs::IfWeak::overwrite() {
		if (created)
			return created.ret();

		Auto<Str> name;
		if (varName)
			name = varName->v;
		if (!name)
			name = weakCast->overwrite();
		if (!name)
			return null;

		created = CREATE(LocalVar, this, name->v, weakCast->result(), weakCast->pos);
		return created.ret();
	}

	void bs::IfWeak::blockCode(Par<CodeGen> state, Par<CodeResult> r) {
		using namespace code;

		// We can create the 'created' variable here. It is fine, since it is not visible in the
		// 'false' branch anyway, and its lifetime is not going to hurt, since it does not live more
		// than a few instructions extra compared to the correct version.
		if (created)
			created->create(state);

		Auto<CodeResult> cond = CREATE(CodeResult, this, boolType(engine()), state->block);
		weakCast->code(state, cond, created);

		Label elseLbl = state->to.label();
		Label doneLbl = state->to.label();

		state->to << cmp(cond->location(state).var(), byteConst(0));
		state->to << jmp(elseLbl, ifEqual);

		Value rType = result().type();
		Auto<Expr> t = expectCastTo(trueCode, rType);
		t->code(state, r);

		if (falseCode) {
			state->to << jmp(doneLbl);
		}

		state->to << elseLbl;

		if (falseCode) {
			Auto<Expr> t = expectCastTo(falseCode, rType);
			t->code(state, r);
		}

		state->to << doneLbl;
	}


	/**
	 * Create appropriate if statement.
	 */

	bs::Expr *bs::createIf(Par<Block> parent, Par<Expr> cond) {
		Value exprResult = cond->result().type().asRef(false);

		if (exprResult == boolType(parent->engine())) {
			return CREATE(If, parent, parent, cond);
		} else if (isMaybe(exprResult)) {
			Auto<WeakCast> cast = CREATE(WeakMaybeCast, cond, cond);
			return CREATE(IfWeak, parent, parent, cast);
		} else {
			throw SyntaxError(cond->pos, L"You can not use the type: " + ::toS(exprResult) + L" in an if-statement!");
		}
	}


	/**
	 * IfTrue
	 */

	bs::IfTrue::IfTrue(Par<Block> parent) : Block(parent) {
		if (IfWeak *weak = as<IfWeak>(parent.borrow())) {
			Auto<LocalVar> override = weak->overwrite();
			if (override) {
				add(override);
			}
		}
	}

	void bs::IfTrue::set(Par<Expr> e) {
		expr = e;
	}

	ExprResult bs::IfTrue::result() {
		return expr->result();
	}

	void bs::IfTrue::blockCode(Par<CodeGen> state, Par<CodeResult> to) {
		expr->code(state, to);
	}

	void bs::IfTrue::output(wostream &to) const {
		to << expr;
	}

}
