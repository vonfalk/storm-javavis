#include "stdafx.h"
#include "BSWeakCast.h"
#include "BSVar.h"
#include "BSNamed.h"
#include "Lib/Maybe.h"
#include "Exception.h"
#include "Engine.h"

namespace storm {

	bs::WeakCast::WeakCast(SrcPos pos) : pos(pos) {}

	MAYBE(Str) *bs::WeakCast::overwrite() {
		return null;
	}

	MAYBE(Str) *bs::WeakCast::defaultOverwrite(Par<Expr> expr) {
		if (LocalVarAccess *var = as<LocalVarAccess>(expr.borrow()))
			return CREATE(Str, this, var->var->name);

		// TODO? Enforce 'this' is used as the parameter?
		if (MemberVarAccess *var = as<MemberVarAccess>(expr.borrow()))
			return CREATE(Str, this, var->var->name);

		return null;
	}

	Value bs::WeakCast::result() {
		return Value();
	}

	void bs::WeakCast::code(Par<CodeGen> state, Par<CodeResult> boolResult, MAYBE(Par<LocalVar>) var) {
		using namespace code;

		// Always indicate cast failed.
		state->to << mov(boolResult->location(state).var(), byteConst(0));
	}


	/**
	 * 'as' cast.
	 */

	bs::WeakDowncast::WeakDowncast(Par<Block> block, Par<Expr> expr, Par<SrcName> type)
		: WeakCast(expr->pos), expr(expr) {

		to = block->scope.value(type).type;

		Value from = expr->result().type();
		if (isMaybe(from))
			from = unwrapMaybe(from);

		if (!from.isClass())
			throw SyntaxError(expr->pos, L"The 'as' operator is only applicable to class or actor types.");
		if (!to.type->isA(from.type))
			throw SyntaxError(expr->pos, L"Condition is always false. " + ::toS(to) +
							L" does not inherit from " + ::toS(from) + L".");
	}

	MAYBE(Str) *bs::WeakDowncast::overwrite() {
		return defaultOverwrite(expr);
	}

	Value bs::WeakDowncast::result() {
		return to;
	}

	void bs::WeakDowncast::code(Par<CodeGen> state, Par<CodeResult> boolResult, MAYBE(Par<LocalVar>) var) {
		using namespace code;

		// Get the value...
		Value fromType = expr->result().type();
		Auto<CodeResult> from = CREATE(CodeResult, this, fromType, state->block);
		expr->code(state, from);

		// Load into eax...
		if (fromType.ref) {
			state->to << mov(ptrA, from->location(state).var());
			state->to << mov(ptrA, ptrRel(ptrA));
		} else {
			state->to << mov(ptrA, from->location(state).var());
		}

		// Store the object into 'var' regardless if the test succeeded. It does not matter.
		if (var) {
			state->to << mov(var->var.var(), ptrA);
			state->to << code::addRef(ptrA);
			var->var.created(state);
		}

		// Call the 'as' function.
		state->to << fnParam(ptrA);
		state->to << fnParam(to.type->typeRef);
		state->to << fnCall(engine().fnRefs.asFn, retVal(Size::sByte, false));
		state->to << mov(boolResult->location(state).var(), al);
	}

	void bs::WeakDowncast::output(wostream &to) const {
		to << expr << " as " << this->to;
	}

	/**
	 * MaybeCast.
	 */

	bs::WeakMaybeCast::WeakMaybeCast(Par<Expr> expr) : WeakCast(expr->pos), expr(expr) {}

	MAYBE(Str) *bs::WeakMaybeCast::overwrite() {
		return defaultOverwrite(expr);
	}

	Value bs::WeakMaybeCast::result() {
		return unwrapMaybe(expr->result().type());
	}

	void bs::WeakMaybeCast::code(Par<CodeGen> state, Par<CodeResult> boolResult, MAYBE(Par<LocalVar>) var) {
		using namespace code;

		// Note: we're assuming we're working with Maybe<T> where T is a class.

		// Get the variable and store it in our variable.
		Auto<CodeResult> from = CREATE(CodeResult, this, expr->result().type().asRef(false), var->var);
		expr->code(state, from);

		// Check if it is null.
		state->to << cmp(var->var.var(), intPtrConst(0));
		state->to << setCond(boolResult->location(state).var(), ifNotEqual);
	}

	void bs::WeakMaybeCast::output(wostream &to) const {
		to << expr;
	}


	/**
	 * Helpers.
	 */

	bs::WeakCast *bs::weakAsCast(Par<Block> block, Par<Expr> expr, Par<SrcName> type) {
		// Currently, only downcast is supported.
		return CREATE(WeakDowncast, expr, block, expr, type);
	}

}
