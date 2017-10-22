#include "stdafx.h"
#include "WeakCast.h"
#include "Named.h"
#include "Compiler/Exception.h"
#include "Compiler/Engine.h"
#include "Compiler/Lib/Maybe.h"

namespace storm {
	namespace bs {

		WeakCast::WeakCast(SrcPos pos) : pos(pos) {}

		MAYBE(Str *) WeakCast::overwrite() {
			return null;
		}

		MAYBE(Str *) WeakCast::defaultOverwrite(Expr *expr) {
			if (LocalVarAccess *var = as<LocalVarAccess>(expr))
				return new (this) Str(*var->var->name);

			// TODO? Enforce 'this' is used as the parameter?
			if (MemberVarAccess *var = as<MemberVarAccess>(expr))
				return new (this) Str(*var->var->name);

			return null;
		}

		Value WeakCast::result() {
			return Value();
		}

		void WeakCast::code(CodeGen *state, CodeResult *boolResult, MAYBE(LocalVar *) var) {
			using namespace code;

			// Always indicate cast failed.
			*state->l << mov(boolResult->location(state).v, byteConst(0));
		}


		/**
		 * 'as' cast.
		 */

		WeakDowncast::WeakDowncast(Block *block, Expr *expr, SrcName *type)
			: WeakCast(expr->pos), expr(expr) {

			to = block->scope.value(type);

			Value from = expr->result().type();
			if (isMaybe(from))
				from = unwrapMaybe(from);

			if (!from.isHeapObj())
				throw SyntaxError(expr->pos, L"The 'as' operator is only applicable to class or actor types.");
			if (!to.type->isA(from.type))
				throw SyntaxError(expr->pos, L"Condition is always false. " + ::toS(to) +
								L" does not inherit from " + ::toS(from) + L".");
		}

		MAYBE(Str *) WeakDowncast::overwrite() {
			return defaultOverwrite(expr);
		}

		Value WeakDowncast::result() {
			return to;
		}

		void WeakDowncast::code(CodeGen *state, CodeResult *boolResult, MAYBE(LocalVar *) var) {
			using namespace code;

			// Get the value...
			Value fromType = expr->result().type();
			CodeResult *from = new (this) CodeResult(fromType, state->block);
			expr->code(state, from);

			// Load into eax...
			if (fromType.ref) {
				*state->l << mov(ptrA, from->location(state).v);
				*state->l << mov(ptrA, ptrRel(ptrA, Offset()));
			} else {
				*state->l << mov(ptrA, from->location(state).v);
			}

			// Store the object into 'var' regardless if the test succeeded. It does not matter.
			if (var) {
				*state->l << mov(var->var.v, ptrA);
				var->var.created(state);
			}

			// Call the 'as' function.
			*state->l << fnParam(engine().ptrDesc(), ptrA);
			*state->l << fnParam(engine().ptrDesc(), to.type->typeRef());
			*state->l << fnCall(engine().ref(Engine::rAs), false, engine().ptrDesc(), ptrA);
			*state->l << cmp(ptrA, ptrConst(Offset()));
			*state->l << setCond(al, ifNotEqual);
			*state->l << mov(boolResult->location(state).v, al);
		}

		void WeakDowncast::toS(StrBuf *to) const {
			*to << expr << " as " << this->to;
		}

		/**
		 * MaybeCast.
		 */

		WeakMaybeCast::WeakMaybeCast(Expr *expr) : WeakCast(expr->pos), expr(expr) {}

		MAYBE(Str *) WeakMaybeCast::overwrite() {
			return defaultOverwrite(expr);
		}

		Value WeakMaybeCast::result() {
			return unwrapMaybe(expr->result().type());
		}

		void WeakMaybeCast::code(CodeGen *state, CodeResult *boolResult, MAYBE(LocalVar *) var) {
			using namespace code;

			// Note: we're assuming we're working with Maybe<T> where T is a class.

			// Get the variable and store it in our variable.
			CodeResult *from = new (this) CodeResult(expr->result().type().asRef(false), var->var);
			expr->code(state, from);

			// Check if it is null.
			*state->l << cmp(var->var.v, ptrConst(Offset()));
			*state->l << setCond(boolResult->location(state).v, ifNotEqual);
		}

		void WeakMaybeCast::toS(StrBuf *to) const {
			*to << expr;
		}


		/**
		 * Helpers.
		 */

		WeakCast *weakAsCast(Block *block, Expr *expr, SrcName *type) {
			// Currently, only downcast is supported.
			return new (expr) WeakDowncast(block, expr, type);
		}


	}
}
