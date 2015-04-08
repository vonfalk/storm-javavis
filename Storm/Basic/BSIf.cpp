#include "stdafx.h"
#include "BSIf.h"
#include "Exception.h"
#include "BSNamed.h"

namespace storm {

	bs::If::If(Par<Block> parent) : Block(parent) {}

	void bs::If::cond(Par<Expr> e) {
		if (e->result() != Value(boolType(engine())))
			throw TypeError(e->pos, L"The expression must evaluate to Bool.");
		condition = e;
	}

	void bs::If::trueExpr(Par<Expr> e) {
		trueCode = e;
	}

	void bs::If::falseExpr(Par<Expr> e) {
		falseCode = e;
	}

	Value bs::If::result() {
		if (falseCode && trueCode) {
			Value t = trueCode->result();
			Value f = falseCode->result();
			return common(t, f);
		} else {
			return Value();
		}
	}

	void bs::If::blockCode(const GenState &state, GenResult &r) {
		using namespace code;

		GenResult condResult(Value::stdBool(engine()), state.block);
		condition->code(state, condResult);

		Label lblElse = state.to.label();
		Label lblDone = state.to.label();

		state.to << cmp(condResult.location(state).var(), byteConst(0));
		state.to << jmp(lblElse, ifEqual);

		trueCode->code(state, r);

		if (falseCode) {
			state.to << jmp(lblDone);
			state.to << lblElse;

			falseCode->code(state, r);

			state.to << lblDone;
		} else {
			state.to << lblElse;
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

		Auto<LocalVarAccess> var = expression.as<LocalVarAccess>();
		if (var)
			created = CREATE(LocalVar, this, var->var->name, Value(target), var->pos);
		return created.ret();
	}

	void bs::IfAs::trueExpr(Par<IfAsTrue> e) {
		trueCode = e;
	}

	void bs::IfAs::falseExpr(Par<Expr> e) {
		falseCode = e;
	}

	Value bs::IfAs::result() {
		if (falseCode && trueCode) {
			Value t = trueCode->result();
			Value f = falseCode->result();
			return common(t, f);
		} else {
			return Value();
		}
	}

	void bs::IfAs::validate() {
		Value eResult = expression->result();
		if (!eResult.isClass())
			throw SyntaxError(pos, L"The as operator is only applicable to class types.");
		if (!target->isA(eResult.type))
			throw SyntaxError(pos, L"Condition is always false. " + target->identifier() + L" does not inherit from "
							+ eResult.type->identifier() + L".");
		if (target == eResult.type)
			throw SyntaxError(pos, L"Condition is always true. The type of the expression is the same as "
							L"the one specified (" + target->identifier() + L").");
	}

	void bs::IfAs::blockCode(const GenState &state, GenResult &r) {
		using namespace code;

		validate();

		Value eResult = expression->result();
		GenResult expr(expression->result(), state.block);
		expression->code(state, expr);
		Variable exprVar = expr.location(state).var();

		Label lblElse = state.to.label();
		Label lblDone = state.to.label();

		if (eResult.ref) {
			state.to << mov(ptrA, exprVar);
			state.to << mov(ptrA, ptrRel(ptrA));
			state.to << fnParam(ptrA);
		} else {
			state.to << fnParam(exprVar);
		}
		state.to << fnParam(target->typeRef);
		state.to << fnCall(engine().fnRefs.asFn, Size::sByte);
		state.to << cmp(al, byteConst(0));
		state.to << jmp(lblElse, ifEqual);

		if (created) {
			// We cheat and create the variable here. It does not hurt, since it is not visible
			// in the false branch anyway.
			created->create(state);
			if (eResult.ref) {
				state.to << mov(ptrA, exprVar);
				state.to << mov(created->var.var(), ptrRel(ptrA));
			} else {
				state.to << mov(created->var.var(), exprVar);
			}
			state.to << code::addRef(created->var.var());
			created->var.created(state);
		}

		trueCode->code(state, r);
		if (falseCode) {
			state.to << jmp(lblDone);
			state.to << lblElse;

			falseCode->code(state, r);

			state.to << lblDone;
		} else {
			state.to << lblElse;
		}
	}

	bs::IfAsTrue::IfAsTrue(Par<IfAs> parent) : Block(parent) {
		Auto<LocalVar> override = parent->override();
		if (override) {
			add(override);
		}
	}

	void bs::IfAsTrue::set(Par<Expr> e) {
		expr = e;
	}

	Value bs::IfAsTrue::result() {
		return expr->result();
	}

	void bs::IfAsTrue::blockCode(const GenState &state, GenResult &to) {
		expr->code(state, to);
	}

}
