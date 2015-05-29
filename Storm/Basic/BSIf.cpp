#include "stdafx.h"
#include "BSIf.h"
#include "BSAutocast.h"
#include "Exception.h"
#include "BSNamed.h"
#include "Lib/Maybe.h"
#include "Engine.h"

namespace storm {

	bs::If::If(Par<Block> parent) : Block(parent) {}

	bs::LocalVar *bs::If::override() {
		if (created)
			return created.ret();

		String varName;
		if (LocalVarAccess *var = as<LocalVarAccess>(condition.borrow()))
			varName = var->var->name;
		else if (MemberVarAccess *var = as<MemberVarAccess>(condition.borrow()))
			varName = var->var->name;

		if (!varName.empty()) {
			Value t = condition->result();
			if (MaybeType *m = as<MaybeType>(t.type)) {
				created = CREATE(LocalVar, this, varName, m->param, condition->pos);
			}
		}
		return created.ret();
	}

	void bs::If::cond(Par<Expr> e) {
		// TODO: Some kind of general interface for this?
		Value t = e->result();
		if (as<MaybeType>(t.type))
			;
		else if (e->result().asRef(false) == Value(boolType(engine())))
			;
		else
			throw TypeError(e->pos, L"The expression must evaluate to Bool or Maybe<T>.");

		condition = e;
	}

	void bs::If::trueExpr(Par<IfTrue> e) {
		trueCode = e;
	}

	void bs::If::falseExpr(Par<Expr> e) {
		falseCode = e;
	}

	Value bs::If::result() {
		if (falseCode && trueCode) {
			return common(trueCode, falseCode);
		} else {
			return Value();
		}
	}

	void bs::If::blockCode(Par<CodeGen> state, Par<CodeResult> r) {
		using namespace code;

		Value condType = condition->result().asRef(false);
		Auto<CodeResult> condResult = CREATE(CodeResult, this, condType, state->block);
		condition->code(state, condResult);

		Label lblElse = state->to.label();
		Label lblDone = state->to.label();

		VarInfo c = condResult->location(state);
		if (as<MaybeType>(condType.type)) {
			state->to << cmp(c.var(), natPtrConst(0));
			state->to << jmp(lblElse, ifEqual);
			if (created) {
				// We cheat and create the variable here. It does not hurt, since it is not visible
				// in the false branch anyway.
				created->create(state);
				state->to << mov(created->var.var(), c.var());
				if (condType.refcounted())
					state->to << code::addRef(created->var.var());
				created->var.created(state);
			}
		} else {
			state->to << cmp(condResult->location(state).var(), byteConst(0));
			state->to << jmp(lblElse, ifEqual);
		}

		Value rType = result();
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
	 * IfAs
	 */

	bs::IfAs::IfAs(Par<Block> parent) : Block(parent) {}

	void bs::IfAs::expr(Par<Expr> expr) {
		expression = expr;
	}

	void bs::IfAs::type(Par<TypeName> type) {
		target = capture(type->resolve(scope).type);
	}

	bs::LocalVar *bs::IfAs::override() {
		if (created)
			return created.ret();

		validate();

		String varName;
		if (LocalVarAccess *var = as<LocalVarAccess>(expression.borrow()))
			varName = var->var->name;
		else if (MemberVarAccess *var = as<MemberVarAccess>(expression.borrow()))
			varName = var->var->name;

		if (!varName.empty())
			created = CREATE(LocalVar, this, varName, Value(target), expression->pos);
		return created.ret();
	}

	void bs::IfAs::trueExpr(Par<IfTrue> e) {
		trueCode = e;
	}

	void bs::IfAs::falseExpr(Par<Expr> e) {
		falseCode = e;
	}

	Value bs::IfAs::result() {
		if (falseCode && trueCode) {
			return common(trueCode, falseCode);
		} else {
			return Value();
		}
	}

	Value bs::IfAs::eValue() {
		Value r = expression->result();
		if (MaybeType *t = as<MaybeType>(r.type)) {
			r = t->param;
		}
		return r;
	}

	void bs::IfAs::validate() {
		Value eResult = eValue();
		if (!eResult.isClass())
			throw SyntaxError(pos, L"The as operator is only applicable to class types.");
		if (!target->isA(eResult.type))
			throw SyntaxError(pos, L"Condition is always false. " + target->identifier() + L" does not inherit from "
							+ eResult.type->identifier() + L".");
		if (target == eResult.type)
			throw SyntaxError(pos, L"Condition is always true. The type of the expression is the same as "
							L"the one specified (" + target->identifier() + L").");
	}

	void bs::IfAs::blockCode(Par<CodeGen> state, Par<CodeResult> r) {
		using namespace code;

		validate();

		Value eResult = eValue();
		Auto<CodeResult> expr = CREATE(CodeResult, this, expression->result().asRef(eResult.ref), state->block);
		expression->code(state, expr);
		Variable exprVar = expr->location(state).var();

		Label lblElse = state->to.label();
		Label lblDone = state->to.label();

		if (eResult.ref) {
			state->to << mov(ptrA, exprVar);
			state->to << mov(ptrA, ptrRel(ptrA));
			state->to << fnParam(ptrA);
		} else {
			state->to << fnParam(exprVar);
		}
		state->to << fnParam(target->typeRef);
		state->to << fnCall(engine().fnRefs.asFn, Size::sByte);
		state->to << cmp(al, byteConst(0));
		state->to << jmp(lblElse, ifEqual);

		if (created) {
			// We cheat and create the variable here. It does not hurt, since it is not visible
			// in the false branch anyway.
			created->create(state);
			if (eResult.ref) {
				state->to << mov(ptrA, exprVar);
				state->to << mov(created->var.var(), ptrRel(ptrA));
			} else {
				state->to << mov(created->var.var(), exprVar);
			}
			state->to << code::addRef(created->var.var());
			created->var.created(state);
		}

		Value rType = result();
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

	void bs::IfAs::output(wostream &to) const {
		to << L"if (" << expression << L") ";
		to << trueCode;
		if (falseCode) {
			to << L" else ";
			to << falseCode;
		}
	}


	bs::IfTrue::IfTrue(Par<IfAs> parent) : Block(parent) {
		Auto<LocalVar> override = parent->override();
		if (override) {
			add(override);
		}
	}

	bs::IfTrue::IfTrue(Par<If> parent) : Block(parent) {
		Auto<LocalVar> override = parent->override();
		if (override) {
			add(override);
		}
	}

	void bs::IfTrue::set(Par<Expr> e) {
		expr = e;
	}

	Value bs::IfTrue::result() {
		return expr->result();
	}

	void bs::IfTrue::blockCode(Par<CodeGen> state, Par<CodeResult> to) {
		expr->code(state, to);
	}

	void bs::IfTrue::output(wostream &to) const {
		to << expr;
	}

}
